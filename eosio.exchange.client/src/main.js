import Vue from 'vue'
import App from './App.vue'
import VueRouter from 'vue-router'
import Vuex, { mapState } from 'vuex'
import createPersistedState from 'vuex-persistedstate'
import vuetify from './plugins/vuetify';
import VueClipboard from 'vue-clipboard2'

import Landing from './components/Landing'
import Dashboard from './components/Markets/Dashboard'
import AccountInfo from './components/Account/AccountInfo'
import MarketInfo from './components/Trading/MarketInfo'
import Swap from './components/Swap/Swap'
import Listings from './components/Listings/Listings'
import About from './components/About/About'

Vue.use(VueRouter)
Vue.use(Vuex)

VueClipboard.config.autoSetContainer = true
Vue.use(VueClipboard)

Vue.config.productionTip = false

const store = new Vuex.Store({
  state: {
    accountPageIsActive: false,
    marketPageIsActive: true,
    listingPageIsActive: false,
    aboutPageIsActive: false,
    endpoint: 'https://jungle.eosusa.news:443',
    trader1alice_PrivateKey: '5JLL42ShsECocztJmQReVmWPk9MmgXMkun9DgQQokY6nk5mrXJZ',
    trader2barry_PrivateKey: '5JLL42ShsECocztJmQReVmWPk9MmgXMkun9DgQQokY6nk5mrXJZ',
    account: null,
    privateKey: null,
    market: null,
    quote: null,
  },
  mutations: {
    setEndpoint (state, endpoint) {
      state.endpoint = endpoint
    },
    setAccount(state, account) {
      state.account = account
    },
    setAliceKey(state, privateKey) {
      state.trader1alice_PrivateKey = privateKey
    },
    setBarryKey(state, privateKey) {
      state.trader2barry_PrivateKey = privateKey
    },
    setPrivateKey(state, privateKey) {
      state.privateKey = privateKey
    },
    setAccountPageActive(state, accountPageIsActive) {
      state.accountPageIsActive = accountPageIsActive
    },
    setMarketPageActive(state, marketPageIsActive) {
      state.marketPageIsActive = marketPageIsActive
    },
    setListingPageActive(state, listingPageIsActive) {
      state.listingPageIsActive = listingPageIsActive
    },
    setAboutPageActive(state, aboutPageIsActive) {
      state.aboutPageIsActive = aboutPageIsActive
    },
  },
  actions: {
    setEndpoint({ commit }, payload) {
      commit('setEndpoint', payload)
    },
    setAccount({ commit }, payload) {
      commit('setAccount', payload)
    },
    setAliceKey({ commit }, payload) {
      commit('setAliceKey', payload)
    },
    setBarryKey({ commit }, payload) {
      commit('setBarryKey', payload)
    },
    setPrivateKey({ commit }, payload) {
      commit('setPrivateKey', payload)
    },
    setAccountPageActive({ commit }, payload) {
      commit('setAccountPageActive', payload)
    },
    setMarketPageActive({ commit }, payload) {
      commit('setMarketPageActive', payload)
    },
    setListingPageActive({ commit }, payload) {
      commit('setListingPageActive', payload)
    },
    setAboutPageActive({ commit }, payload) {
      commit('setAboutPageActive', payload)
    },
  },
  getters: {
    endpoint: state => state.endpoint,
    alicePrivateKey: state => state.trader1alice_PrivateKey,
    barryPrivateKey: state => state.trader2barry_PrivateKey,
    privateKey: state => state.privateKey,
    account: state => state.account,
    accountPageIsActive: state => state.accountPageIsActive,
    marketPageIsActive: state => state.marketPageIsActive,
    listingPageIsActive: state => state.listingPageIsActive,
    aboutPageIsActive: state => state.aboutPageIsActive,
  },
  plugins: [
    createPersistedState({
      storage: window.sessionStorage,
    })
  ]
})

const routes = [
  { name: 'landing', path: '/', component: Landing },
  { name: 'dashboard', path: '/dashboard', component: Dashboard },
  { name: 'account', path: '/account', component: AccountInfo },
  { name: 'market', path: '/market', component: MarketInfo },
  { name: 'swap', path: '/swap', component: Swap },
  { name: 'listings', path: '/listings', component: Listings },
  { name: 'about', path: '/about', component: About },
]

const router = new VueRouter({
  routes,
  mode: 'history'
})

new Vue({
  vuetify,
  router,
  store,
  computed: mapState([
    'endpoint', 'trader1alice_PrivateKey', 'trader2barry_PrivateKey',
  ]),
  render: h => h(App)
}).$mount('#app')
