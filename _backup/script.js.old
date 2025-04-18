// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

// Create UVIndex Gauge
var guageWaterLevel = new LinearGauge({
  renderTo: 'gauge-waterlevel',
  units: "%",
  width: 120, height: 400,
  minValue: 0, maxValue: 100,
  valueDec: 2, valueInt: 2,
  colorValueBoxRect: "#049faa", colorValueBoxRectEnd: "#049faa", colorValueBoxBackground: "#f1fbfc",
  majorTicks: ["0","10","20","30","40","50","60","70","80","90","100"],
  minorTicks: 2,
  highlights: [ { "from": 75, "to": 100, "color": "rgba(100, 100, 200, .75)" },],
  colorPlate: "#fff", colorBarProgress: "#f0f0f0", colorBarProgressEnd: "#101010",
  borderShadowWidth: 0, borders: false,
  needleType: "arrow", needleWidth: 4, needleCircleSize: 2, needleCircleOuter: true, needleCircleInner: false,
  animationDuration: 1500, animationRule: "elastic",
  barWidth: 10,
}).draw();
  
// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      guageWaterLevel.value = myObj.waterlevel;
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
}

if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    guageWaterLevel.value = myObj.waterlevel;
  }, false);
}