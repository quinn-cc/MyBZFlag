# Score Restorer

[![GitHub release](https://img.shields.io/github/release/allejo/ScoreRestorer.svg)](https://github.com/allejo/ScoreRestorer/releases/latest)
![Minimum BZFlag Version](https://img.shields.io/badge/BZFlag-v2.4.14+-blue.svg)
[![License](https://img.shields.io/github/license/allejo/ScoreRestorer.svg)](LICENSE.md)

A BZFlag plug-in that restores a player's score whenever they rejoin within a set amount of time. This plug-in also allows admins to modify a player's score with a slash command.

## Requirements

- BZFlag 2.4.14+
- C++11

This plug-in follows [my standard instructions for compiling plug-ins](https://github.com/allejo/docs.allejo.io/wiki/BZFlag-Plug-in-Distribution).

## Usage

### Loading the plug-in

This plug-in does not take any configuration options at load time.

```
-loadplugin ScoreRestorer
```

### Custom BZDB Variables

These custom BZDB variables can be configured with `-set` in configuration files and may be changed at any time in-game by using the `/set` command.

```
-set <name> <value>
```

| Name | Type | Default | Description |
| ---- | ---- | ------- | ----------- |
| `_scoreSaveTime` | int | 120 | The amount of seconds a player's score will be saved after they've left. |

## License

[MIT](LICENSE.md)
