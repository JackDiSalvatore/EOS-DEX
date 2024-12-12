<template>

  <v-app-bar
    class="appBar"
    color="background"
    dark
  >
    <!-- <v-app-bar-nav-icon></v-app-bar-nav-icon> -->

    <img
      :src="require('../assets/logo_OCI_cube_full-color_946x946.png')"
      alt="logo"
      height="75"
    >
    <v-toolbar-title class="pageTitle">
      <a @click="gotoLanding" style="text-transform: uppercase;">E<span style="font-size: 0.8em">xchange</span></a>
      <span class="demo-text" style="font-size: 0.7em !important;margin-left: 7px;">Demo</span>
    </v-toolbar-title>

    <v-spacer></v-spacer>

    <a
      class="navItem"
      v-bind:class="{ navItemActive: accountPageIsActive }"
      @click="gotoAccount"
    >
      {{ $store.getters.account }}
    </a>

    <a
      class="navItem"
      v-bind:class="{ navItemActive: marketPageIsActive }"
      @click="gotoMarkets"
    >
      Markets
    </a>

    <a
      class="navItem"
      v-bind:class="{ navItemActive: listingPageIsActive }"
      @click="gotoListings"
    >
      Listings
    </a>

    <a
      class="navItem"
      v-bind:class="{ navItemActive: aboutPageIsActive }"
      @click="gotoAbout"
    >
      About
    </a>

  </v-app-bar>

</template>

<script>

export default {
  name: 'Nav',

  components: {

  },
  data: () => ({

  }),
  computed: {
    accountPageIsActive: function () {
      return this.$store.getters.accountPageIsActive
    },
    marketPageIsActive: function () {
      return this.$store.getters.marketPageIsActive
    },
    listingPageIsActive: function () {
      return this.$store.getters.listingPageIsActive
    },
    aboutPageIsActive: function () {
      return this.$store.getters.aboutPageIsActive
    }
  },
  mounted () {

  },
  methods: {
    gotoLanding() {
      this.$router.push('/')
    },
    gotoAccount() {
      this.$store.dispatch('setMarketPageActive', false)
      this.$store.dispatch('setAccountPageActive', true)
      this.$store.dispatch('setListingPageActive', false)
      this.$store.dispatch('setAboutPageActive', false)

      this.$router.push('/account')
    },
    gotoMarkets() {
      this.$store.dispatch('setAccountPageActive', false)
      this.$store.dispatch('setMarketPageActive', true)
      this.$store.dispatch('setListingPageActive', false)
      this.$store.dispatch('setAboutPageActive', false)

      this.$router.push('/dashboard')
    },
    gotoListings() {
      this.$store.dispatch('setAccountPageActive', false)
      this.$store.dispatch('setMarketPageActive', false)
      this.$store.dispatch('setListingPageActive', true)
      this.$store.dispatch('setAboutPageActive', false)

      this.$router.push('/listings')
    },
    gotoAbout() {
      this.$store.dispatch('setAccountPageActive', false)
      this.$store.dispatch('setMarketPageActive', false)
      this.$store.dispatch('setListingPageActive', false)
      this.$store.dispatch('setAboutPageActive', true)

      this.$router.push('/about')
    }
  }
};
</script>

<style scoped>
.appBar {
  box-shadow: none;
}
.pageTitle {
  color: var(--v-primary-base);
  /* font-weight: bold; */
  letter-spacing: 0.02em;
  font-size: 1.5em;
}

.navItem {
  font-size: 1.2em;
  text-transform: uppercase;
  letter-spacing: 0.02em;
  color: var(--v-secondary-base) !important;
  padding: 0 5px 0 5px;
}

.navItemActive {
  color: var(--v-primary-base) !important;
}
</style>