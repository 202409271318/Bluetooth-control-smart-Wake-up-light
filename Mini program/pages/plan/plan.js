const ble = require('../../utils/ble.js');

Page({
  data: {
    sleepLight: '23:00',
    sleepTurn:  '23:20',
    sleepDark:  '23:40',
    wakeStart:  '07:00',
    wakeMax:    '07:30',
    wakeOff:    '08:00',
    sleepSliderDisabled: false,
    wakeSliderDisabled: false
  },

  bindTime(e) {
    const key = e.currentTarget.dataset.key;
    this.setData({ [key]: e.detail.value }, () => {
      this.validateTime('sleep');
      this.validateTime('wake');
    });
  },

  // 验证时间是否有效（熄灭时间必须晚于亮起时间）
  validateTime(type) {
    const lightKey = type === 'sleep' ? 'sleepLight' : 'wakeStart';
    const darkKey = type === 'sleep' ? 'sleepDark' : 'wakeOff';
    const turnKey = type === 'sleep' ? 'sleepTurn' : 'wakeMax';
    const disabledKey = type === 'sleep' ? 'sleepSliderDisabled' : 'wakeSliderDisabled';
    
    const lightTime = this.data[lightKey];
    const darkTime = this.data[darkKey];
    
    const lightMinutes = this.timeToMinutes(lightTime);
    const darkMinutes = this.timeToMinutes(darkTime);
    
    if (darkMinutes <= lightMinutes) {
      // 熄灭时间早于或等于亮起时间，报错并禁用转折时间设置
      wx.showToast({ 
        title: '熄灭时间必须晚于亮起时间', 
        icon: 'none', 
        duration: 2000 
      });
      this.setData({ [disabledKey]: true });
    } else {
      this.setData({ [disabledKey]: false });
      // 验证转折时间是否在有效范围内
      const turnTime = this.data[turnKey];
      const turnMinutes = this.timeToMinutes(turnTime);
      if (turnMinutes <= lightMinutes || turnMinutes >= darkMinutes) {
        // 重置转折时间为中间值
        const midMinutes = Math.floor((lightMinutes + darkMinutes) / 2);
        const midTime = this.minutesToTime(midMinutes);
        this.setData({ [turnKey]: midTime });
      }
    }
  },

  // 将时间字符串转换为分钟数
  timeToMinutes(time) {
    const parts = time.split(':');
    return parseInt(parts[0]) * 60 + parseInt(parts[1]);
  },

  // 将分钟数转换为时间字符串
  minutesToTime(minutes) {
    const h = Math.floor(minutes / 60);
    const m = minutes % 60;
    return `${h.toString().padStart(2, '0')}:${m.toString().padStart(2, '0')}`;
  },

  // 设置转折时间
  bindTurnTime(e) {
    const key = e.currentTarget.dataset.key;
    const type = key.includes('sleep') ? 'sleep' : 'wake';
    const disabledKey = type === 'sleep' ? 'sleepSliderDisabled' : 'wakeSliderDisabled';
    
    if (this.data[disabledKey]) {
      wx.showToast({ 
        title: '请先修正亮起/熄灭时间', 
        icon: 'none', 
        duration: 2000 
      });
      return;
    }
    
    this.setData({ [key]: e.detail.value }, () => {
      // 显示转折时间意义的提示
      const hint = type === 'sleep' 
        ? '转折时间：灯光从常亮转为渐暗的时刻' 
        : '转折时间：灯光达到最大亮度的时刻';
      wx.showToast({ 
        title: hint, 
        icon: 'none', 
        duration: 2500 
      });
    });
  },

  async saveSleep() {
    // 保存前再次验证
    if (!this.validateAndShowError('sleep')) {
      return;
    }
    try {
      await ble.setSleepPlan(this.data.sleepLight,
                             this.data.sleepTurn,
                             this.data.sleepDark);
      wx.showToast({ title: '睡眠计划已保存' });
    } catch (e) {
      wx.showToast({ title: '保存失败', icon: 'none' });
    }
  },

  async saveWake() {
    // 保存前再次验证
    if (!this.validateAndShowError('wake')) {
      return;
    }
    try {
      await ble.setWakePlan(this.data.wakeStart,
                            this.data.wakeMax,
                            this.data.wakeOff);
      wx.showToast({ title: '起床计划已保存' });
    } catch (e) {
      wx.showToast({ title: '保存失败', icon: 'none' });
    }
  },

  validateAndShowError(type) {
    const lightKey = type === 'sleep' ? 'sleepLight' : 'wakeStart';
    const darkKey = type === 'sleep' ? 'sleepDark' : 'wakeOff';
    
    const lightMinutes = this.timeToMinutes(this.data[lightKey]);
    const darkMinutes = this.timeToMinutes(this.data[darkKey]);
    
    if (darkMinutes <= lightMinutes) {
      wx.showToast({ 
        title: '熄灭时间必须晚于亮起时间', 
        icon: 'none', 
        duration: 2000 
      });
      return false;
    }
    return true;
  },

  async saveAll() {
    // 保存前验证两个计划
    if (!this.validateAndShowError('sleep') || !this.validateAndShowError('wake')) {
      return;
    }
    try {
      const c = s => s.replace(':', '');
      await ble.sendCommands([
        `SLEEP=${c(this.data.sleepLight)},${c(this.data.sleepTurn)},${c(this.data.sleepDark)}`,
        `WAKE=${c(this.data.wakeStart)},${c(this.data.wakeMax)},${c(this.data.wakeOff)}`
      ]);
      wx.showToast({ title: '全部保存成功' });
    } catch (e) {
      wx.showToast({ title: '保存失败', icon: 'none' });
    }
  },

  goBack() {
    wx.navigateBack();
  }
});
