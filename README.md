Загрузка проштвки через curl
```
curl -v -X POST -F update=@firmware.bin http://192.168.56.5/update
```

Compress js, css
```
google-closure-compiler --js ./html/script.js --js_output_file ./data/script.mini.js
```