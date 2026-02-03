# Eqnx S-eXpressions

Language designed to estructure the app given plugins and their arguments.

## Structure 
- **atom**: `atom`, plugin name or constant
- **expression**: `(atom s1 s2 ...)`, where s1 is the first argument of atom, and it's an expression.
- *separators*: It's recommended to use space ` ` (or newline), but `|` can be used.
- *constants*: C constants (int, float, ...)

## Example
```esx
(vsplit 
    (picker)
    (color 0xFF0000)
)
```

or

```esx 
(vsplit|(picker)|(color|0xFF0000))
```
