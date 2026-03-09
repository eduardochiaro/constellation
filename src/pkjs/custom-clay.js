module.exports = function(minified) {
  var clayConfig = this;
  var _ = minified._;
  var $ = minified.$;
  var HTML = minified.HTML;

  function toggleStepTracker() {
    if (this.get()) {
      clayConfig.getItemByMessageKey('STEP_GOAL').enable();
    } else {
      clayConfig.getItemByMessageKey('STEP_GOAL').disable();
    }
  }

  function toggleWeather() {
    if (this.get()) {
      clayConfig.getItemByMessageKey('WEATHER_SCALE').enable();
    } else {
      clayConfig.getItemByMessageKey('WEATHER_SCALE').disable();
    }
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var stepTrackerToggle = clayConfig.getItemByMessageKey('SHOW_STEP_TRACKER');
    toggleStepTracker.call(stepTrackerToggle);
    stepTrackerToggle.on('change', toggleStepTracker);

    var weatherToggle = clayConfig.getItemByMessageKey('SHOW_WEATHER');
    toggleWeather.call(weatherToggle);
    weatherToggle.on('change', toggleWeather);
  });
};