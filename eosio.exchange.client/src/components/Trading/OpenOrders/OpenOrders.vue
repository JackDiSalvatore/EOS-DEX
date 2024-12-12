<template>

  <v-card outlined tile class="pa-3" :class="className">

    <v-row>
      <v-col cols="12" align="start" justify="center">
        <div style="display: flex;">
          <h3 style="padding-right: 10px;">Open Orders</h3>
          <v-btn-toggle
            v-model="toggle_buy_sell"
            color="accent"
            mandatory
          >
            <v-btn small class="buttonClass" style="border: 1px solid #4A90E2 !important;">
              Buys
            </v-btn>
            <v-btn small class="buttonClass" style="border: 1px solid #4A90E2 !important;">
              Sells
            </v-btn>
          </v-btn-toggle>
        </div>
      </v-col>
    </v-row>

    <v-row>
      <v-col cols="12" v-if="!toggle_buy_sell">
        <OpenBuyOrders
          v-bind:endpoint="endpoint"
          v-bind:account="account"
          v-bind:privateKey="privateKey"
          v-bind:market="market"
          v-bind:base="base"
          v-bind:quote="quote"
          v-bind:bids="bids"
          v-bind:asks="asks"
        />
      </v-col>
      <v-col cols="12" v-if="toggle_buy_sell">
        <OpenSellOrders
          v-bind:endpoint="endpoint"
          v-bind:account="account"
          v-bind:privateKey="privateKey"
          v-bind:market="market"
          v-bind:base="base"
          v-bind:quote="quote"
          v-bind:bids="bids"
          v-bind:asks="asks"
        />
      </v-col>
    </v-row>

  </v-card>

</template>

<script>
import OpenBuyOrders from './OpenBuyOrders';
import OpenSellOrders from './OpenSellOrders';

export default {
  name: 'OpenOrders',
  props: ['endpoint', 'account', 'privateKey', 'market', 'base', 'quote', 'bids', 'asks', 'className'],
  components: {
    OpenBuyOrders,
    OpenSellOrders,
  },
  data: () => ({
    toggle_buy_sell: 0,
  }),
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