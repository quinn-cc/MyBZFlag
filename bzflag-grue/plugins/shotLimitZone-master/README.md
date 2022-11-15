# Shot Limit Zone

[![GitHub release](https://img.shields.io/github/release/allejo/shotLimitZone.svg)](https://github.com/allejo/shotLimitZone/releases/latest)
![Minimum BZFlag Version](https://img.shields.io/badge/BZFlag-v2.4.4+-blue.svg)
[![License](https://img.shields.io/github/license/allejo/shotLimitZone.svg)](LICENSE.md)

A BZFlag plugin that allows you add shot limits to flag based on the location of the flag. The difference between this plugin and the default BZFS `-sl` option is that by using the option, you are limiting all of the flags of the same type where with this plugin you will only limit the shot limit of a flag if grabbed from a specific area in the map. This plugin will not overwrite the `-sl` option.

## Requirements

- BZFlag 2.4.4+

This plug-in follows [my standard instructions for compiling plug-ins](https://github.com/allejo/docs.allejo.io/wiki/BZFlag-Plug-in-Distribution).

## Usage

### Loading the plug-in

This plug-in does not take any configuration options at load time.

```
-loadplugin shotLimitZone
```

### Custom Map Objects

This plug-in introduces the `SHOTLIMITZONE` map object which supports the traditional `position`, `size`, and `rotation` attributes for rectangular objects and `position`, `height`, and `radius` for cylindrical objects.

**Box**

```
shotLimitZone
  position 0 0 0
  size 10 10 5
  rotation 0
  shotLimit 10
  flag SW
end
```

**Cylinder**

```
shotLimitZone
  position 0 0 0
  radius 15
  height 10
  shotLimit 10
  flag SW
end
```

#### Notes

- This custom zone will not actually spawn a SW flag, you will need to add a regular zone object to do so.
- The name ('shotLimitZone') of the object is case-insensitive so camel case is not required.

## License

[MIT](LICENSE.md)
