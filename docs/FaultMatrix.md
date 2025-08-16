| Fault | Detection | Immediate Action | Blocks Service Ops? | Clear Condition | Audit Entry |
|------|-----------|------------------|---------------------|-----------------|-------------|
| ESTOP | E-stop input asserted | Motors disabled, shutter closed | Yes | Physical reset | Yes |
| LID_TAMPER | Lid switch open | Shutter forced closed, dispense blocked | Yes | Lid closed and stable | Yes |
| OVERCURRENT | Overcurrent input asserted | Motors disabled | Yes until clear | Condition clears | Yes |
| JAM_SHUTTER | Program logic | Attempt jam-clear profile | No | Jam-clear success | Yes |
| JAM_HOPPER | Program logic | Attempt jam-clear profile | No | Jam-clear success or service | Yes |
| LIMIT_TIMEOUT | Program logic | Fault | No | Service investigation | Yes |
| SENSOR_FAIL | Program logic | Fault | Yes | Fix sensor | Yes |
| CONFIG_ERROR | Startup/config | Fault | Yes | Fix config | Yes |
