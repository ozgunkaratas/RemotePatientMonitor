# tests — integration and hardware-in-the-loop

System-level tests that span more than one component, added from phase 1 on:

- simulator scenarios run end to end with their expected alarms (integration)
- Mega + Pi together with fault injection: link loss, watchdog (hardware in the loop)
- latency and jitter measurements with the logic analyzer (phase 5)

Unit tests stay next to their component (`protocol/tests`, `gateway/tests`).
