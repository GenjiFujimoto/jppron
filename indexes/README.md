
These are a couple indexes I created for the forvo files of the corresponding username.
The kana reading is simple filled with the file name.

The expected folder structure is as follows:

kaoring
   ├── index.json
   └── media
       ├── 橫書き.opus
       ├── 每日.opus
       ├── 踌躇い.opus


These were simply generated with the commands:
```
ls | sed 's|\(.*\).opus|        "\1": [\n            "&"\n        ],|' >> ../index.json
ls | sed 's|\(.*\).opus|        "&": {\n            "kana_reading": "\1"\n        },|' >> ../index.json
```
and some manual editing
