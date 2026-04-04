// GridSpace Configuration
var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var customClay = require('./custom-clay');
var clay = new Clay(clayConfig, customClay);

// ============================================================================
// Weather Data Functions
// ============================================================================

// Calculate moon phase (0 = new moon, 0.5 = full moon, 1 = new moon)
function calculateMoonPhase(date) {
  var year = date.getFullYear();
  var month = date.getMonth() + 1;
  var day = date.getDate();
  
  if (month < 3) {
    year--;
    month += 12;
  }
  
  var a = Math.floor(year / 100);
  var b = Math.floor(a / 4);
  var c = 2 - a + b;
  var e = Math.floor(365.25 * (year + 4716));
  var f = Math.floor(30.6001 * (month + 1));
  var jd = c + day + e + f - 1524.5;
  
  var daysSinceNew = jd - 2451549.5;
  var newMoons = daysSinceNew / 29.53;
  var phase = newMoons - Math.floor(newMoons);
  
  return phase;
}

// Get moon phase name and code representation
function getMoonPhaseInfo(phase) {
  var phaseNames = [
    { name: 'New Moon', icon: 0, min: 0, max: 0.033 },
    { name: 'Waxing Crescent',icon: 1, min: 0.033, max: 0.216 },
    { name: 'First Quarter', icon: 2, min: 0.216, max: 0.283 },
    { name: 'Waxing Gibbous', icon: 3, min: 0.283, max: 0.466 },
    { name: 'Full Moon', icon: 4, min: 0.466, max: 0.533 },
    { name: 'Waning Gibbous', icon: 5, min: 0.533, max: 0.716 },
    { name: 'Last Quarter', icon: 6, min: 0.716, max: 0.783 },
    { name: 'Waning Crescent', icon: 7, min: 0.783, max: 0.967 },
    { name: 'New Moon', icon: 0, min: 0.967, max: 1.0 }
  ];
  
  for (var i = 0; i < phaseNames.length; i++) {
    if (phase >= phaseNames[i].min && phase < phaseNames[i].max) {
      return phaseNames[i];
    }
  }
  
  return phaseNames[0];
}

// Fetch weather data from Open-Meteo API
function fetchWeatherData(latitude, longitude) {
  console.log('Fetching weather for: ' + latitude + ', ' + longitude);
  
  // Open-Meteo API URL with all required data
  var url = 'https://api.open-meteo.com/v1/forecast?' +
    'latitude=' + latitude +
    '&longitude=' + longitude +
    '&current=temperature_2m,weather_code' +
    '&daily=sunrise,sunset' +
    '&timezone=auto' +
    '&forecast_days=1';
  
  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  
  xhr.onload = function() {
    if (xhr.status === 200) {
      try {
        var response = JSON.parse(xhr.responseText);
        console.log('Weather data received:', JSON.stringify(response));
        
        // Calculate moon phase
        var now = new Date();
        var moonPhase = calculateMoonPhase(now);
        var moonInfo = getMoonPhaseInfo(moonPhase);
        
        // Create weather data object
        var weatherData = {
          temperature: Math.round(response.current.temperature_2m),
          weatherCode: response.current.weather_code,
          sunrise: response.daily.sunrise[0],
          sunset: response.daily.sunset[0],
          moonPhase: Math.round(moonPhase * 100), // 0-100
          moonPhaseName: moonInfo.name,
          moonPhaseIcon: moonInfo.icon,
          timestamp: Date.now()
        };
        
        console.log('Sending weather data:', JSON.stringify(weatherData));
        
        // Send to watchface as JSON string
        Pebble.sendAppMessage({
          'WEATHER_DATA': JSON.stringify(weatherData)
        }, function() {
          console.log('Weather data sent successfully');
        }, function(e) {
          console.log('Failed to send weather data: ' + JSON.stringify(e));
        });
        
      } catch (e) {
        console.log('Error parsing weather data: ' + e.message);
      }
    } else {
      console.log('Weather request failed: ' + xhr.status);
    }
  };
  
  xhr.onerror = function() {
    console.log('Weather request error');
  };
  
  xhr.send();
}

// Get location and fetch weather
function updateWeather() {
  // Skip weather fetch if both weather display and moon view are disabled
  try {
    var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
    var showWeather = settings.SHOW_WEATHER;
    var showMoonView = settings.SHOW_MOON_VIEW;
    // Clay stores toggles as true/false; treat missing as default (true)
    if (showWeather === false && showMoonView === false) {
      console.log('Weather and moon view both disabled, skipping fetch');
      return;
    }
  } catch (e) {}

  console.log('Getting location for weather update...');
  
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      console.log('Location obtained: ' + pos.coords.latitude + ', ' + pos.coords.longitude);
      fetchWeatherData(pos.coords.latitude, pos.coords.longitude);
    },
    function(err) {
      console.log('Location error: ' + err.message);
    },
    { timeout: 15000, maximumAge: 600000 }
  );
}

// ============================================================================
// Pebble Event Handlers
// ============================================================================

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  
  // Fetch weather on startup
  updateWeather();
  
  // Update weather every 60 minutes
  setInterval(updateWeather, 60 * 60 * 1000);
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('Message from watchface:', JSON.stringify(e.payload));
  
  // If watchface requests weather update
  if (e.payload.REQUEST_WEATHER) {
    updateWeather();
  }
});