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

    <PlaceBuyOrder
      v-if="!toggle_buy_sell"
      v-bind:endpoint="endpoint"
      v-bind:account="account"
      v-bind:privateKey="privateKey"
      v-bind:market="this.market"
      v-bind:quote="quote"
    />

    <PlaceSellOrder
      v-if="toggle_buy_sell"
      v-bind:endpoint="endpoint"
      v-bind:account="account"
      v-bind:privateKey="privateKey"
      v-bind:market="this.market"
      v-bind:quote="quote"
    />

  </v-card>

</template>

<script>
import PlaceBuyOrder from './PlaceBuyOrder';
import PlaceSellOrder from './PlaceSellOrder';

export default {
  name: 'PlaceOrders',
  props: ['endpoint', 'account', 'privateKey', 'market', 'quote', 'className'],
  components: {
    PlaceBuyOrder,
    PlaceSellOrder,
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