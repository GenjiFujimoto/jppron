This is a work in progress application to play a pronunciation of a Japanese word, using a local database
in the [ajatt audio format](https://github.com/Ajatt-Tools/nhk_2016_pronunciations_index).

## Installation
`sudo make install`

## Usage
`jppron word [reading]`. The very first run might take a while, since it will create an index saved in 
`$XDG_DATA_HOME/jppron/`. 

Currently it is expecting the audio file directories to be stored at `$XDB_DATA_HOME/ajt_japanese_audio/`
with file structure:
```
├── ajt_japanese_audio
│   ├── daijisen_pronunciations_index
│   │   ├── index.json
│   │   ├── LICENSE
│   │   ├── media
│   │   ├── README.md
│   │   └── release_info.conf
│   ├── kaoring
│   │   ├── index.json
│   │   └── media
│   ├── nhk_1998_pronunciations_index
│   │   ├── index.json
│   │   ├── media
│   │   ├── README.md
│   │   └── release_info.conf
│   ├── nhk_2016_pronunciations_index
│   │   ├── index.json
│   │   └── media
...
```
I will make this more flexible soon.
