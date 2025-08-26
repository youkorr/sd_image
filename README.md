
direct reading on the sd card in big_endian and little_endian the program cannot resize the images it is up to you to do it and the files must be in jpg format

```
storage:
    platform: sd_direct
    id: my_storage
    sd_component: sd_card
    root_path: "/sdcard" 
    sd_images:
      - id: testree
        file_path: "/img/sanctuary.jpg"
        resize: 1280x720
        format: rgb565
        byte_order: little_endian 
        auto_load: true
```
