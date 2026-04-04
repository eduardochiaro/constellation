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

  function setLogoDisplay() {
    var splashLogoStyle = clayConfig.getItemByMessageKey('SPLASH_LOGO_STYLE');
    const image = () => {
      switch (splashLogoStyle.get()) {
        case '1': return 'https://raw.githubusercontent.com/eduardochiaro/constellation/main/shared/resources/splash_logos/constellation_bw_logo.png';
        case '2': return 'https://raw.githubusercontent.com/eduardochiaro/constellation/main/shared/resources/splash_logos/constellation_color_logo.png';
        case '3': return 'https://raw.githubusercontent.com/eduardochiaro/constellation/main/shared/resources/splash_logos/house_varuun_logo.png';
        case '4': return 'https://raw.githubusercontent.com/eduardochiaro/constellation/main/shared/resources/splash_logos/freestar_logo.png';
        case '5': return 'https://raw.githubusercontent.com/eduardochiaro/constellation/main/shared/resources/splash_logos/sysdef_logo.png';
        case '6': return 'https://raw.githubusercontent.com/eduardochiaro/constellation/main/shared/resources/splash_logos/crimson_logo.png';
        case '7': return 'https://raw.githubusercontent.com/eduardochiaro/constellation/main/shared/resources/moon/moon_background.png';
        case '8': return 'https://raw.githubusercontent.com/eduardochiaro/constellation/main/shared/resources/splash_logos/tracker_alliance_bw_logo.png';
        case '9': return 'https://raw.githubusercontent.com/eduardochiaro/constellation/main/shared/resources/splash_logos/tracker_alliance_color_logo.png';
        default: return null;
      }
    }
    const currentImage = image();
    if (currentImage) {
      clayConfig.getItemById('SPLASH_LOGO').set("<img src='" + currentImage + "' style='max-width:100%; max-height:100%; margin: 0 auto; display: block;' />");
    } else {
      clayConfig.getItemById('SPLASH_LOGO').set('Splash Logo will not be displayed');
    }
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var stepTrackerToggle = clayConfig.getItemByMessageKey('SHOW_STEP_TRACKER');
    toggleStepTracker.call(stepTrackerToggle);
    stepTrackerToggle.on('change', toggleStepTracker);

    var weatherToggle = clayConfig.getItemByMessageKey('SHOW_WEATHER');
    toggleWeather.call(weatherToggle);
    weatherToggle.on('change', toggleWeather);



    var splashLogoStyle = clayConfig.getItemByMessageKey('SPLASH_LOGO_STYLE');
    setLogoDisplay.call(splashLogoStyle);
    splashLogoStyle.on('change', setLogoDisplay);
  });
};