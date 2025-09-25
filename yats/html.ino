// start page
const char* startPage = u8R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>yats</title>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial; margin: 40px; }
        button { padding: 10px 20px; margin: 5px; font-size: 16px; }
    </style>
</head>
<body>
    <h1>yats Webserver</h1>
    <p>Temperatur: <span id="current_temp">--.-</span>°C</p>
    <p>Luftfeuchtigkeit: <span id="current_humi">--.-</span>%</p>
    <p>Datum/Uhrzeit: <span id="current_datetime">--.--.-- --:--</span></p>
    <br>
    <br>
    <p>Minimale Temperatur: <span id="min_temp">--.-</span>°C, <span id="min_temp_datetime">--.--.-- --:--</span></p>
    <p>Maximale Temperatur: <span id="max_temp">--.-</span>°C, <span id="max_temp_datetime">--.--.-- --:--</span></p>
    <br>
    <br>
    <p>Minimale Feuchtigkeit: <span id="min_humi">--.-</span>%, <span id="min_humi_datetime">--.--.-- --:--</span></p>
    <p>Maximale Feuchtigkeit: <span id="max_humi">--.-</span>%, <span id="max_humi_datetime">--.--.-- --:--</span></p>
    <button onclick="updateSensors()">Aktualisieren</button>

    <script>
        function updateSensors() {
            fetch('/sensors')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('current_temp').innerText = data.current_temp;
                    document.getElementById('current_humi').innerText = data.current_humi;
                    document.getElementById('current_datetime').innerText = data.current_datetime;
                    document.getElementById('min_temp').innerText = data.min_temp;
                    document.getElementById('min_temp_datetime').innerText = data.min_temp_datetime;
                    document.getElementById('max_temp').innerText = data.max_temp;
                    document.getElementById('max_temp_datetime').innerText = data.max_temp_datetime;
                    document.getElementById('min_humi').innerText = data.min_humi;
                    document.getElementById('min_humi_datetime').innerText = data.min_humi_datetime;
                    document.getElementById('max_humi').innerText = data.max_humi;
                    document.getElementById('max_humi_datetime').innerText = data.max_humi_datetime;
                });
        }
        
        // Status beim Laden der Seite aktualisieren
        updateSensors();
    </script>
</body>
</html>
)=====";