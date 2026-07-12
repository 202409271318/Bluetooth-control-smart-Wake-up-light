const ble = require('../../utils/ble.js');

// 7个预设颜色（高饱和度版本）
const COLOR_PRESETS = [
 { name: '红色', r: 255, g: 50, b: 50 },
 { name: '橙色', r: 255, g: 150, b: 0 },
 { name: '黄色', r: 255, g: 255, b: 0 },
 { name: '绿色', r: 50, g: 255, b: 50 },
 { name: '蓝色', r: 50, g: 150, b: 255 },
 { name: '紫色', r: 200, g: 50, b: 255 },
 { name: '白色', r: 255, g: 255, b: 255 }
 ];

Page({
  data: {
    activeIdx: 0,
    modes: [
      { r: 255, g: 160, b: 60,  br: 80  },
      { r: 80,  g: 180, b: 255, br: 100 },
      { r: 180, g: 80,  b: 255, br: 60  }
    ],
    colorPresets: COLOR_PRESETS,
    selectedPresetIndex: -1,
    brightnessPercent: 31
  },

  onLoad() {
    this.updateBrightnessPercent();
  },

  updateBrightnessPercent() {
    const br = this.data.modes[this.data.activeIdx].br;
    this.setData({ brightnessPercent: Math.round(br / 2.55) });
  },

  switchTab(e) {
    const idx = parseInt(e.currentTarget.dataset.idx);
    this.setData({ activeIdx: idx });
    this.updateBrightnessPercent();
  },

  selectPreset(e) {
    const idx = parseInt(e.currentTarget.dataset.idx);
    // 如果点击的是已选中的预设，则取消选中
    if (this.data.selectedPresetIndex === idx) {
      this.setData({ selectedPresetIndex: -1 });
      return;
    }
    const preset = COLOR_PRESETS[idx];
    const activeIdx = this.data.activeIdx;
    const modes = this.data.modes.slice();
    modes[activeIdx] = Object.assign({}, modes[activeIdx], {
      r: preset.r,
      g: preset.g,
      b: preset.b
    });
    this.setData({ 
      modes,
      selectedPresetIndex: idx 
    });
  },

  changeChannel(e) {
    const ch  = e.currentTarget.dataset.ch;
    const val = e.detail.value;
    const i   = this.data.activeIdx;
    const modes = this.data.modes.slice();
    modes[i] = Object.assign({}, modes[i], { [ch]: val });
    // 如果修改了RGB值，取消预设选中状态
    if (ch === 'r' || ch === 'g' || ch === 'b') {
      this.setData({ modes, selectedPresetIndex: -1 });
    } else {
      this.setData({ modes });
    }
    // 更新亮度百分比
    this.updateBrightnessPercent();
  },

  async saveCurrent() {
    const i = this.data.activeIdx;
    const m = this.data.modes[i];
    try {
      await ble.setMode(i + 1, m.r, m.g, m.b, m.br);
      wx.showToast({ title: `模式 ${i + 1} 已保存` });
    } catch (e) {
      wx.showToast({ title: '保存失败', icon: 'none' });
    }
  },

  async saveAll() {
    try {
      const cmds = this.data.modes.map((m, i) =>
        `MODE${i + 1}=${m.r},${m.g},${m.b},${m.br}`);
      await ble.sendCommands(cmds);
      wx.showToast({ title: '全部已保存' });
    } catch (e) {
      wx.showToast({ title: '保存失败', icon: 'none' });
    }
  },

  goBack() {
    wx.navigateBack();
  }
});
