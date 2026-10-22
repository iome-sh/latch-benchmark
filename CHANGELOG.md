# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). This project has no tagged release yet. Versions will follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html) once the first one is tagged.

## [Unreleased]

### Added

- MuJoCo harness (`harness/mujoco`) with the mock-oracle checklist runner `scripts/fa_checklist_run.sh`.
- Harnesses for ROS 2 control, ROS 2 with gz, gz control, behaviour-tree conditions and mock election, with CI workflows.
- Public 60-row golden fixture `fixtures/latch-robot-1.jsonl`. CI now runs it.
- Schema notes for evidence records, golden JSONL and mode records, plus a citation file, a security policy and contributing guidelines.

### Changed

- The suite token is renamed to `latch-benchmark`.
- The citation and security contacts now point at this repository.
- The SPEC status is split into readable lines.

### Removed

- The certificate disclaimer in verify, soak and the checklist output.
