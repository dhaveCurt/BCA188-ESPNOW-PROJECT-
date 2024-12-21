#ifndef PAGEINDEX_H
#define PAGEINDEX_H

const char PAGEINDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Sensor Data</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #B37A4C; /* Background Color */
      margin: 0;
      padding: 0;
    }

    .container {
      text-align: center;
      padding: 50px;
    }

    .status-box {
      border: 2px solid #CC9966; /* Border Color */
      padding: 30px; /* Increased padding for larger box */
      border-radius: 15px;
      box-shadow: 2px 2px 15px rgba(204, 153, 102, 0.5); /* Shadow Effect */
      background-color: #FFFDDD; /* Box Background */
      font-size: 24px; /* Larger font size */
      font-weight: bold;
      color: #B37A4C; /* Text Color */
      width: 400px; /* Increased width */
      margin: 20px auto; /* Added margin for spacing */
      text-align: center;
    }

    .status-box p {
      margin: 0;
    }

    .title {
      font-size: 36px;
      font-weight: bold;
      color: #FFFDDD;
      text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.5);
      margin-bottom: 20px;
    }
  </style>
<script>
  // Function to fetch the latest data
  function fetchData() {
    // Fetch Sound Status
    fetch("/status/sound")
      .then(response => response.text())
      .then(data => {
        document.getElementById("soundStatus").innerText = "Sound Status: " + data;
      })
      .catch(error => {
        console.error('Error fetching sound data:', error);
        document.getElementById("soundStatus").innerText = "Sound Status: Error";
      });

    // Fetch Motion Status
    fetch("/status/motion")
      .then(response => response.text())
      .then(data => {
        document.getElementById("motionStatus").innerText = "Motion Sensor: " + data;
      })
      .catch(error => {
        console.error('Error fetching motion data:', error);
        document.getElementById("motionStatus").innerText = "Motion Sensor: Error";
      });

    // Fetch Smoke Status
    fetch("/status/smoke")
      .then(response => response.text())
      .then(data => {
        document.getElementById("smokeStatus").innerText = "Smoke Status: " + data;
      })
      .catch(error => {
        console.error('Error fetching smoke data:', error);
        document.getElementById("smokeStatus").innerText = "Smoke Status: Error";
      });

    // Fetch Light Level and Brightness Percentage
    fetch("/status/light")
      .then(response => response.json())
      .then(data => {
        document.getElementById("brightnessPercentage").innerText = "Brightness: " + data.brightnessPercentage;
      })
      .catch(error => {
        console.error('Error fetching light data:', error);
        document.getElementById("brightnessPercentage").innerText = "Brightness: Error";
      });
  }

  // Refresh data every 2 seconds
  setInterval(fetchData, 2000);
</script>

</head>
<body>
  <div class="container">
    <div class="title">My Smart Home Office</div>

    <div class="status-box" id="soundStatus">Sound Status: Loading...</div>
    <div class="status-box" id="motionStatus">Motion Sensor: Loading...</div>
    <div class="status-box" id="smokeStatus">Smoke Status: Loading...</div>
    <div class="status-box" id="brightnessPercentage">Brightness: Loading...</div>
  </div>
</body>
</html>
)rawliteral";

#endif
