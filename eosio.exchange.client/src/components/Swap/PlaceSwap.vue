<template>

  <v-card outlined tile :class="className">

    <v-row class="ma-3">
      <v-col cols="12" align="center" justify="center">
        <v-btn-toggle
          v-model="toggle_buy_sell"
          color="accent"
          mandatory
        >
          <v-btn small class="buttonClass" style="border: 1px solid #4A90E2 !important;">
            B U Y
          </v-btn>
          <v-btn small class="buttonClass" style="border: 1px solid #4A90E2 !important;">
            S E L L
          </v-btn>
        </v-btn-toggle>
      </v-col>
    </v-row>

    <v-row class="ma-3">
      <v-col
        v-if="!toggle_buy_sell"
        cols="12"
        align="center"
        justify-self="center"
      >
        <SwapBuy
          v-bind:endpoint="endpoint"
          v-bind:account="account"
          v-bind:privateKey="privateKey"
          v-bind:market="this.market"
          v-bind:quote="quote"
          v-bind:price="price"
        />
      </v-col>
      <v-col
        v-if="toggle_buy_sell"
        cols="12"
        align="center"
        justify-self="center"
      >
        <SwapSell
          v-bind:endpoint="endpoint"
          v-bind:account="account"
          v-bind:privateKey="privateKey"
          v-bind:market="this.market"
          v-bind:quote="quote"
          v-bind:price="price"
        />
      </v-col>
    </v-row>

  </v-card>

</template>

<script>
import SwapBuy from './SwapBuy';
import SwapSell from './SwapSell';

export default {
  name: 'PlaceSwap',
  props: ['endpoint', 'account', 'privateKey', 'marketStats', 'market', 'quote', 'className'],
  components: {
    SwapBuy,
    SwapSell,
  },
  data: () => ({
    toggle_buy_sell: 0,
  }),
  computed: {
    price: function () {
      return this.marketStats.price.quantity.replace(/\s[A-Z]+/g, '')
    }
  },
  mounted () {

  },
  methods: {

  }
};
</script>

<style scoped>

.buttonClass {
  background: #003E5C !important;
  color: var(--v-secondary-base);
}

/* .buttonClass:hover {
  background: #003E5C !important;
  color: var(--v-accent-base) !important;
} */

.buttonClass {
  border: 1px solid green !important;
}

</style>