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

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var coolStuffToggle = clayConfig.getItemByMessageKey('SHOW_STEP_TRACKER');
    toggleStepTracker.call(coolStuffToggle);
    coolStuffToggle.on('change', toggleStepTracker);
  });
};