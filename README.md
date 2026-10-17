# ServerMonitor

A real-time server monitoring dashboard built with **Qt6** and **C++17**.

Agents run on remote machines, collect CPU, RAM, and Disk metrics, and send them over TCP to a central dashboard that displays live status for all connected servers. The system includes a dedicated Alert Window that triggers on high resource usage.

![Dashboard](screenshots/dashboard.png)
![Alert Window](screenshots/alert_window.png)


## Features

- Real-time CPU, RAM, and Disk monitoring (absolute + percentage)
- Multi-agent support (tested with 9+ simultaneous agents)
- TCP-based communication between agents and dashboard
- Live status indicators (New / Online / Offline)
- Relative time display (Elapsed column)
- **Real-time Alert Window for resource thresholds (Warning > 80%, Critical > 90%)**
- **Auto-hide Alert Window when all alerts are resolved**
- **Simulate Alert button for easy testing and demos**
- Persistent agent configuration (via QSettings) and auto-generated agent names (via MAC address)
- Save and load agent lists as JSON
- Clear offline or all agents with one click


## Architecture

[Agent 1] ──TCP──┐
[Agent 2] ──TCP──┤
[Agent 3] ──TCP──┼──▶ [ServerMonitor Dashboard]
... │ │
[Agent N] ──TCP──┘ ▼
[Alert Window]
(Triggers on >80% / >90%)


- **ServerAgent**: A lightweight Qt application that runs on each monitored machine, collects local CPU, RAM, and Disk usage, and sends JSON-formatted metrics to the dashboard every 2 seconds.
- **ServerMonitor Dashboard**: A Qt-based GUI that listens for incoming agent connections, parses metrics, and displays them in a real-time table.
- **AlertWindow**: A separate, independent window that monitors incoming metrics. It highlights values that exceed the Warning (80%) or Critical (90%) thresholds and automatically hides itself when no active alerts remain.


## Requirements

- Qt 6.5 or higher
- CMake 3.16+
- C++17 compatible compiler (MinGW or MSVC)
- Windows 10/11 (for real CPU/RAM metrics via Windows API)


## How to Build

Open the project in Qt Creator and press Run.

Or build manually:

mkdir build && cd build
cmake ..
cmake --build .



## How to Use

1. Start the **ServerMonitor Dashboard** — it will listen on port `12345`.
2. Start one or more **ServerAgent** instances (on the same machine or remote machines).
3. In each agent, set the dashboard's IP, port, and click **Connect**.
4. The agent will appear in the dashboard's table and start sending metrics every 2 seconds.
5. If any metric exceeds 80% (Warning) or 90% (Critical), the **Alert Window** will appear automatically.
6. Use the **Simulate Alert** button on the dashboard to test the Alert Window without overloading your system.


## Roadmap

- [x] TCP server and agent communication
- [x] Real CPU, RAM, and Disk metrics via Windows API
- [x] Status coloring and elapsed time display
- [x] Multi-agent support
- [x] Alerts for high resource usage (Warning/Critical)
- [ ] Agent configuration persistence (save IP/port/name)
- [ ] Auto-reconnect on connection loss
- [ ] Cross-platform agent (Linux/macOS)


## License

MIT License - see [LICENSE](LICENSE) file for details.