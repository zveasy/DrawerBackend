# Compliance Plan

## Scope
Pre-compliance activities for register_mvp hardware and firmware prior to formal certification.

## Assumptions
- DUT Hardware Rev A
- Firmware version 1.0.0
- No external peripherals unless listed.

## Samples
| Quantity | Configuration |
|----------|--------------|
| 2 | Production units with latest firmware |
| 1 | Spare control board |

## Lab Equipment
- ESD gun ±15 kV
- EFT/burst generator
- Surge generator
- LISN and spectrum analyzer

## Acceptance Gates
- All scripts and checklists completed
- Pre-scan reports reviewed

## Pre-scan Venues
- In-house chamber
- Local EMC lab pre-scan service

## Definition of Done
- EMC_PreScan_Procedure.md, ESD_Test_Procedure.md and Surge_EFT_Procedure.md complete
- Safety checklist populated and checked
- Lab packet built via `make_lab_packet.sh`
- App supports deterministic worst/idle modes via CLI
- Scripts generate structured logs; validation scripts pass
