# EQNX

Eqnx (equinox) is a library for composing applications by merging independent
plugins. It’s single thread, event-based and easy to use.

Documentation is [here](https://hugocoto.com/projects/eqnx)

> This tool provides the structural foundation for various software extensions
> to seamlessly coexist within a single visual space. Users can program
> autonomous components that not only display information on a character matrix
> but also act as orchestrators for other subcomponents. This visual hierarchy
> grants developers absolute control over the interface layout and interaction
> distribution, ensuring that every element operates in a fluid and coordinated
> manner.

## Calendar

```sh
eqnx -p esx/calendar.esx
```

This plugin let you edit and visualize tasks stored in a toml file. It's the
newest version of [todo](https://github.com/hugoocoto/todo), my old task
manager. 

### Controls

| key | action |
| :--- | :--- |
| `hjkl` | move left, down, up, right |
| `C-n/C-p` | go to the next/previous month |
| `esc` | close the current view |
| `c` | create a task inside a day |

You can use the mouse to click buttons.

### Next steps

- I need a date picker and a color picker to edit the `date` and `fg`,`bg` fields
of the tasks. It works without it, but I want to implement it.
- When reading a day, it would be nice to be able to scroll if all the tasks
  didn't fit in the screen
