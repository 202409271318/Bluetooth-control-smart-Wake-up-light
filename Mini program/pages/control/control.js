const ble = require('../../utils/ble.js');

const DEFAULT_AVATAR_URL = '/images/icons/avatar.png';

Page({
  data: {
    connected: false,
    status: {},
    localTime: '--:--:--',
    localDate: '',
    deviceTime: '--:--',     // 设备返回的时间（用于校对）
    brightLevels: ['关', '低', '中', '高'],
    // 用户信息
    userInfo: {
      avatarUrl: DEFAULT_AVATAR_URL,
      nickName: '微信用户'
    }
  },

  onLoad() {
    this.app = getApp();
    this.loadUserInfo();
    
    this._onStatus = s => {
      console.log('[Control] 收到状态:', s);
      this.setData({
        status: s,
        deviceTime: (s.date ? s.date + ' ' : '') + (s.time || '--:--:--')
      });
    };    
    this._onDisc = () => {
      wx.showToast({ title: '设备已断开', icon: 'none' });
      this.cleanup();
      setTimeout(() => wx.redirectTo({ url: '/pages/index/index' }), 800);
    };
    ble.on('status', this._onStatus);
    ble.on('disconnected', this._onDisc);
    this.setData({ connected: ble.connected, status: ble.status });
  },

  onShow() {
    // 1) 启动本地时钟（每秒刷新界面）
    this.updateLocalTime();
    this.timeTimer = setInterval(() => this.updateLocalTime(), 1000);

    // 2) 立刻拉一次状态
    if (ble.connected) {
      setTimeout(() => ble.getStatus().catch(e => console.warn(e)), 300);
    }

    // 3) 每 5s 主动拉取状态（避免错过 notify）
    this.pollTimer = setInterval(() => {
      if (ble.connected) ble.getStatus().catch(() => {});
    }, 5000);
    
    // 4) 刷新用户信息
    this.loadUserInfo();
  },

  onHide()   { this.cleanup(); },
  onUnload() {
    this.cleanup();
    ble.off('status', this._onStatus);
    ble.off('disconnected', this._onDisc);
  },

  cleanup() {
    if (this.timeTimer) { clearInterval(this.timeTimer); this.timeTimer = null; }
    if (this.pollTimer) { clearInterval(this.pollTimer); this.pollTimer = null; }
  },

  updateLocalTime() {
    const d = new Date();
    const p = n => (n < 10 ? '0' + n : '' + n);
    const localTime = `${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}`;
    const localDate = `${d.getFullYear()}-${p(d.getMonth() + 1)}-${p(d.getDate())}`;
    this.setData({ localTime, localDate });
  },

  /* ============================== 用户信息 ============================== */

  loadUserInfo() {
    const raw = this.app.globalData.userInfo || {};
    const avatarUrl = raw.avatarUrl && String(raw.avatarUrl).trim()
      ? raw.avatarUrl
      : DEFAULT_AVATAR_URL;
    this.setData({ 
      userInfo: Object.assign({}, raw, { avatarUrl }) 
    });
  },

  onSettingAvatarError() {
    if (!this.data.userInfo.avatarUrl || this.data.userInfo.avatarUrl === DEFAULT_AVATAR_URL) return;
    const newUserInfo = Object.assign({}, this.data.userInfo, { avatarUrl: DEFAULT_AVATAR_URL });
    this.app.setUserInfo(newUserInfo);
    this.setData({ userInfo: newUserInfo });
  },

  /* ============================== 同步时间 ============================== */
  async syncTime() {
    wx.showLoading({ title: '同步中...', mask: true });
    try {
      await ble.syncTime();
      // 等设备处理完再拉取
      setTimeout(async () => {
        await ble.getStatus().catch(() => {});
        wx.hideLoading();
        wx.showToast({ title: '✅ 时间已同步', icon: 'success' });
      }, 500);
    } catch (e) {
      wx.hideLoading();
      console.error('同步失败:', e);
      wx.showToast({ title: '同步失败: ' + e.message, icon: 'none' });
    }
  },

  async toggleAuto(e) {
    const on = e.detail.value;
    try {
      await ble.setAuto(on);
      setTimeout(() => ble.getStatus().catch(() => {}), 200);
    } catch (err) {
      wx.showToast({ title: '设置失败', icon: 'none' });
    }
  },

  async tapBright(e) {
    const lv = e.currentTarget.dataset.level;
    try {
      await ble.setBrightLevel(lv);
      setTimeout(() => ble.getStatus().catch(() => {}), 200);
    } catch (err) {
      wx.showToast({ title: '设置失败', icon: 'none' });
    }
  },

  async refresh() {
    wx.showLoading({ title: '刷新中...' });
    try {
      await ble.getStatus();
      setTimeout(() => {
        wx.hideLoading();
        wx.showToast({ title: '已刷新' });
      }, 400);
    } catch (e) {
      wx.hideLoading();
      wx.showToast({ title: '刷新失败', icon: 'none' });
    }
  },

  async disconnect() {
    this.cleanup();
    await ble.disconnect();
    wx.redirectTo({ url: '/pages/index/index' });
  },

  goPlan()   { wx.navigateTo({ url: '/pages/plan/plan' }); },
  goCustom() { wx.navigateTo({ url: '/pages/custom/custom' }); },
  goBack()   { wx.navigateBack(); }
});
