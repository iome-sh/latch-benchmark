/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * Streams a flat JSONL fixture through latch_consider / latch_apply_reject.
 * One JSON object per line. Lines starting with # are comments.
 */
#include "oracle_runner.h"

#include "lbs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { kMaxSeq = 32, kLine = 2048 };

typedef struct SeqSlot {
  char name[40];
  LatchState state;
  int used;
  int seen;
  char prev[16];
} SeqSlot;

static int json_key(const char *line, const char *key, const char **value) {
  char pat[80];
  const char *p = line;
  int n = 0;
  n = snprintf(pat, sizeof pat, "\"%s\"", key);
  if (n <= 0 || (size_t)n >= sizeof pat) return 0;
  while ((p = strstr(p, pat)) != NULL) {
    const char *after = p + (size_t)n;
    while (*after == ' ') ++after;
    if (*after != ':') {
      p += (size_t)n;
      continue;
    }
    ++after;
    while (*after == ' ') ++after;
    *value = after;
    return 1;
  }
  return 0;
}

static int json_string(const char *line, const char *key, char *out, int out_n) {
  const char *value = NULL;
  int n = 0;
  if (!json_key(line, key, &value) || *value != '"') return 0;
  ++value;
  while (*value && *value != '"') {
    if (n + 1 >= out_n) return 0;
    out[n++] = *value++;
  }
  if (*value != '"') return 0;
  out[n] = '\0';
  return 1;
}

static int json_int(const char *line, const char *key, int *out) {
  const char *value = NULL;
  char *end = NULL;
  if (!json_key(line, key, &value)) return 0;
  if (*value == '"') return 0;
  *out = (int)strtol(value, &end, 10);
  return end != value;
}

static int sense_from(const char *line, const char *prefix, LatchSense *sense) {
  static const char *kFields[] = {"joint",    "limit",   "collision", "estop",     "contact",
                                  "grasped",  "target_seen", "sense_age", "wrench"};
  int32_t *slots[9];
  int i = 0;
  memset(sense, 0, sizeof *sense);
  slots[0] = &sense->joint;
  slots[1] = &sense->limit;
  slots[2] = &sense->collision;
  slots[3] = &sense->estop;
  slots[4] = &sense->contact;
  slots[5] = &sense->grasped;
  slots[6] = &sense->target_seen;
  slots[7] = &sense->sense_age;
  slots[8] = &sense->wrench;
  for (i = 0; i < 9; ++i) {
    char key[40];
    int v = 0;
    snprintf(key, sizeof key, "%s%s", prefix, kFields[i]);
    if (!json_int(line, key, &v)) return 0;
    *slots[i] = v;
  }
  return 1;
}

static void note_flip(SeqSlot *slot, const char *seq, const char *word, OracleRun *out) {
  if (strcmp(seq, "chatter") != 0 && strcmp(seq, "raw") != 0) return;
  if (strcmp(seq, "chatter") == 0) out->chatter_rows += 1;
  if (strcmp(seq, "raw") == 0) out->raw_rows += 1;
  if (slot->prev[0] && strcmp(slot->prev, word) != 0) {
    if (strcmp(seq, "chatter") == 0) out->chatter_flips += 1;
    if (strcmp(seq, "raw") == 0) out->raw_flips += 1;
  }
  snprintf(slot->prev, sizeof slot->prev, "%s", word);
}

static SeqSlot *seq_slot(SeqSlot *slots, const char *name) {
  int i = 0;
  for (i = 0; i < kMaxSeq; ++i) {
    if (slots[i].used && strcmp(slots[i].name, name) == 0) return &slots[i];
  }
  for (i = 0; i < kMaxSeq; ++i) {
    if (!slots[i].used) {
      slots[i].used = 1;
      snprintf(slots[i].name, sizeof slots[i].name, "%s", name);
      return &slots[i];
    }
  }
  return NULL;
}

static void rebind(SeqSlot *slot, const LatchBlob *blob, const char *id, OracleRun *out) {
  char msg_id[80];
  memset(&slot->state, 0, sizeof slot->state);
  slot->seen = 0;
  /* prev is the name sequence for flip counts. A reset row starts a new
   * election but the raw sequence still changes name from the previous row. */
  if (latch_bind(&slot->state, blob) != 0) {
    snprintf(msg_id, sizeof msg_id, "%s", id);
    fprintf(stderr, "FAIL %s bind\n", msg_id);
    out->fails += 1;
  }
}

static int hex16(const char *text) {
  int i = 0;
  if (!text || strlen(text) != 16) return 0;
  for (i = 0; i < 16; ++i) {
    char c = text[i];
    int ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    if (!ok) return 0;
  }
  return 1;
}

static void emit_row(LbsOracleEmit emit, void *user, const char *id, const LatchMode *mode, const char *hex) {
  if (!emit) return;
  emit(id, lbs_mode_name(mode->name), lbs_plane_name(mode->plane), hex, user);
}

static void check_mode(const char *id, const LatchMode *got, unsigned name, unsigned plane, int score, int margin,
                       int dwell, const char *evidence, const char *got_hex, int compare_evidence, OracleRun *out) {
  if (got->name != name) {
    fprintf(stderr, "FAIL %s name got=%s expected=%s\n", id, lbs_mode_name(got->name), lbs_mode_name(name));
    out->fails += 1;
  }
  if (got->plane != plane) {
    fprintf(stderr, "FAIL %s plane got=%s expected=%s\n", id, lbs_plane_name(got->plane), lbs_plane_name(plane));
    out->fails += 1;
  }
  if (got->score != score || got->margin != margin || got->dwell != dwell) {
    fprintf(stderr, "FAIL %s quality got=%d/%d/%d expected=%d/%d/%d\n", id, got->score, got->margin, got->dwell, score,
            margin, dwell);
    out->fails += 1;
  }
  if (!compare_evidence) return;
  if (!hex16(got_hex) || strcmp(got_hex, "59a7dbac9a73f1af") == 0) {
    fprintf(stderr, "FAIL %s evidence digest %s\n", id, got_hex);
    out->fails += 1;
  }
  if (!evidence || strcmp(got_hex, evidence) != 0) {
    fprintf(stderr, "FAIL %s evidence got=%s expected=%s\n", id, got_hex, evidence ? evidence : "(missing)");
    out->fails += 1;
  }
}

int lbs_run_oracle_jsonl(const char *path, const char *snapshot_id, int compare_evidence, OracleRun *out) {
  return lbs_replay_oracle_jsonl(path, snapshot_id, compare_evidence, 1, NULL, NULL, out);
}

int lbs_replay_oracle_jsonl(const char *path, const char *snapshot_id, int compare_evidence, int match_fields,
                            LbsOracleEmit emit, void *user, OracleRun *out) {
  FILE *fp = NULL;
  char line[kLine];
  SeqSlot seqs[kMaxSeq];
  LatchBlob blob;
  OracleRun local;
  if (!out) out = &local;
  memset(out, 0, sizeof *out);
  memset(seqs, 0, sizeof seqs);
  memset(&blob, 0, sizeof blob);
  if (!path || !snapshot_id) {
    out->fails += 1;
    return out->fails;
  }
  blob.schema = LATCH_BLOB_SCHEMA;
  blob.snapshot_id = snapshot_id;
  fp = fopen(path, "r");
  if (!fp) {
    fprintf(stderr, "FAIL open oracle %s\n", path);
    out->fails += 1;
    return out->fails;
  }
  while (fgets(line, (int)sizeof line, fp)) {
    char op[16];
    char id[64];
    char evidence[32];
    char name_s[16];
    char plane_s[16];
    unsigned name = 0;
    unsigned plane = 0;
    int score = 0;
    int margin = 0;
    int dwell = 0;
    int have_evidence = 0;
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') continue;
    memset(op, 0, sizeof op);
    memset(id, 0, sizeof id);
    memset(evidence, 0, sizeof evidence);
    if (!json_string(line, "op", op, (int)sizeof op) || !json_string(line, "id", id, (int)sizeof id)) {
      fprintf(stderr, "FAIL parse op/id: %s", line);
      out->fails += 1;
      continue;
    }
    have_evidence = json_string(line, "evidence", evidence, (int)sizeof evidence);
    if (compare_evidence && !have_evidence) {
      fprintf(stderr, "FAIL %s missing evidence\n", id);
      out->fails += 1;
      continue;
    }
    if (!json_string(line, "name", name_s, (int)sizeof name_s) || !lbs_mode_id(name_s, &name) ||
        !json_string(line, "plane", plane_s, (int)sizeof plane_s) || !lbs_plane_id(plane_s, &plane) ||
        !json_int(line, "score", &score) || !json_int(line, "margin", &margin) || !json_int(line, "dwell", &dwell)) {
      fprintf(stderr, "FAIL %s parse mode fields\n", id);
      out->fails += 1;
      continue;
    }
    out->rows += 1;
    if (strcmp(op, "consider") == 0) {
      char seq[40];
      int reset = 0;
      int budget = 0;
      LatchSense sense;
      LatchMode mode;
      SeqSlot *slot = NULL;
      char hex[17];
      uint32_t mask = 0;
      if (!json_string(line, "seq", seq, (int)sizeof seq) || !json_int(line, "reset", &reset) ||
          !json_int(line, "budget", &budget) || !sense_from(line, "", &sense)) {
        fprintf(stderr, "FAIL %s parse consider\n", id);
        out->fails += 1;
        continue;
      }
      slot = seq_slot(seqs, seq);
      if (!slot) {
        fprintf(stderr, "FAIL %s seq capacity\n", id);
        out->fails += 1;
        continue;
      }
      if (reset || !slot->seen) rebind(slot, &blob, id, out);
      memset(&mode, 0, sizeof mode);
      if (latch_consider(&sense, &slot->state, budget, &mode) != 0) {
        fprintf(stderr, "FAIL %s consider rc\n", id);
        out->fails += 1;
        continue;
      }
      latch_evidence(&slot->state, &sense, &mode, hex);
      if (match_fields) {
        check_mode(id, &mode, name, plane, score, margin, dwell, have_evidence ? evidence : NULL, hex,
                   compare_evidence, out);
      }
      emit_row(emit, user, id, &mode, hex);
      mask = latch_legal_mask(&sense);
      if ((mask & (1u << mode.name)) == 0) out->illegal += 1;
      if (mode.plane == LATCH_PLANE_REJECT) out->illegal += 1;
      note_flip(slot, seq, lbs_mode_name(mode.name), out);
      slot->seen = 1;
    } else if (strcmp(op, "apply") == 0) {
      int reject = 0;
      LatchSense decision_sense;
      LatchSense apply_sense;
      LatchMode decision;
      LatchMode applied;
      LatchState fresh;
      char dname[16];
      char dplane[16];
      unsigned dn = 0;
      unsigned dp = 0;
      int dscore = 0;
      int dmargin = 0;
      int ddwell = 0;
      int rc = 0;
      char hex[17];
      if (!json_int(line, "reject", &reject) || !sense_from(line, "", &decision_sense) ||
          !sense_from(line, "a", &apply_sense) || !json_string(line, "dname", dname, (int)sizeof dname) ||
          !lbs_mode_id(dname, &dn) || !json_string(line, "dplane", dplane, (int)sizeof dplane) ||
          !lbs_plane_id(dplane, &dp) || !json_int(line, "dscore", &dscore) || !json_int(line, "dmargin", &dmargin) ||
          !json_int(line, "ddwell", &ddwell)) {
        fprintf(stderr, "FAIL %s parse apply\n", id);
        out->fails += 1;
        continue;
      }
      memset(&decision, 0, sizeof decision);
      decision.name = (uint8_t)dn;
      decision.plane = (uint8_t)dp;
      decision.score = dscore;
      decision.margin = dmargin;
      decision.dwell = ddwell;
      memset(&applied, 0, sizeof applied);
      rc = latch_apply_reject(&decision_sense, &apply_sense, &decision, &applied);
      if (rc != reject) {
        fprintf(stderr, "FAIL %s apply rc got=%d expected=%d\n", id, rc, reject);
        out->fails += 1;
      }
      memset(&fresh, 0, sizeof fresh);
      if (latch_bind(&fresh, &blob) != 0) {
        fprintf(stderr, "FAIL %s apply bind\n", id);
        out->fails += 1;
        continue;
      }
      latch_evidence(&fresh, &apply_sense, &applied, hex);
      if (match_fields) {
        check_mode(id, &applied, name, plane, score, margin, dwell, have_evidence ? evidence : NULL, hex,
                   compare_evidence, out);
      }
      emit_row(emit, user, id, &applied, hex);
    } else {
      fprintf(stderr, "FAIL %s unknown op %s\n", id, op);
      out->fails += 1;
    }
  }
  fclose(fp);
  return out->fails;
}
