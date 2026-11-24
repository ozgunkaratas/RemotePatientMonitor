# System requirements — v0

Status: draft, phase 0. Values marked *(tune)* are first guesses to be tuned with the simulator.
IDs never change once assigned; removed requirements are marked *withdrawn*, not deleted.

Each test names the requirement IDs it verifies (see `docs/traceability.md`, generated later).

## Safety and scope

| ID | Requirement | Verified by |
|---|---|---|
| SYS-001 | No sensor attached to a person shall be electrically connected to mains-powered equipment; body-worn sensors run on battery and transmit wirelessly. | Design review each phase |
| SYS-002 | The system is a learning system; its outputs shall not be used for clinical decisions. The central station shall show this notice at start-up. | Inspection |

## Data acquisition (Mega)

| ID | Requirement | Verified by |
|---|---|---|
| SYS-010 | The Mega shall sample each simulated waveform at 250 Hz with a sampling period jitter below 50 µs. | Logic analyzer (phase 5) |
| SYS-011 | Every frame from the Mega shall carry a sequence number and the Mega's microsecond timestamp of the first sample. | Unit test, protocol |
| SYS-012 | The simulator shall provide selectable scenarios: normal, tachycardia, bradycardia, asystole, desaturation, lead off, sensor dropout. | Integration test |
| SYS-013 | The Mega shall send a HEARTBEAT at 10 Hz. | Integration test |

## Communication

| ID | Requirement | Verified by |
|---|---|---|
| SYS-020 | Mega-Pi frames shall be COBS-framed with a CRC-16/CCITT-FALSE; frames with a bad CRC shall be dropped and counted. | Unit test, fault injection |
| SYS-021 | The gateway shall detect lost frames from sequence gaps and count them. | Unit test |
| SYS-022 | Loss of the Mega heartbeat for more than 1 s shall raise a medium-priority technical alarm. *(tune)* | Fault injection |
| SYS-023 | The gateway shall treat data as valid only after the first HEARTBEAT following a port open (the Mega resets on open). | Integration test |
| SYS-024 | The Pi-desktop link shall use TLS with mutual authentication from phase 8. | Security test |

## Analysis and alarms (Pi)

| ID | Requirement | Verified by |
|---|---|---|
| SYS-030 | Heart rate derived from the ECG shall be within ±2 bpm of the simulator ground truth in all non-artifact scenarios. | Integration test |
| SYS-031 | Each alarm condition shall be handled by its own state machine: Normal, Pending, Active, Acknowledged. | Unit test |
| SYS-032 | Asystole (no beat for 4 s) shall raise a latching high-priority alarm. *(tune)* | Integration test |
| SYS-033 | Acknowledge shall silence audio only; the alarm shall stay visible while its condition is true. | Integration test |
| SYS-034 | From the end of the alarm condition delay, an alarm shall be shown on the desktop and on the bedside panel within 500 ms. | Logic analyzer (phase 5) |

## Closed-loop control (phase 6)

| ID | Requirement | Verified by |
|---|---|---|
| SYS-040 | If the Mega receives no Pi heartbeat within the watchdog timeout, it shall drive the valve to its safe default position on its own. | HIL fault injection |
| SYS-041 | The controller output shall be limited in step size and range before it is sent to the valve. | Unit test |
| SYS-042 | A manual override button on the bedside panel shall take precedence over the controller. | HIL test |
