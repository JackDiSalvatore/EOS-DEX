<template>

  <v-card
    class="pa-3 order-book"
    outlined
    tile
    :class="className"
  >

    <v-simple-table fixed-header dense style="height: 8%;">
      <template v-slot:default>
        <thead>
            <tr>
            <th class="text-left">Price ({{ quote.symbol }})</th>
            <th class="text-left">Volume ({{ base }})</th>
            </tr>
        </thead>
      </template>
    </v-simple-table>

    <!-- <v-simple-table fixed-header dense style="height: 43%;display: flex; align-items: flex-end;"> -->
    <v-simple-table fixed-header dense style="height: 43%;align-items: flex-end;display: flex;">
      <template v-slot:default>
        <tbody>
          <tr v-for="item in totalBids" :key="item.id">
            <td class="sellText bookItem">{{ item.price.quantity.replace(/\s[A-Z]+/g, '') }}</td>
            <td class="bookItem">{{ item.volume }}</td>
          </tr>
        </tbody>
      </template>
    </v-simple-table>

    <div class="price">
      {{ marketStats ? marketStats.price.quantity : null }}
    </div>

    <v-simple-table fixed-header dense style="height: 43%;">
      <template v-slot:default>
        <tbody>
            <tr v-for="item in totalAsks" :key="item.id">
            <td class="buyText bookItem">{{ item.price.quantity.replace(/\s[A-Z]+/g, '') }}</td>
            <td class="bookItem">{{ item.volume }}</td>
            </tr>
        </tbody>
      </template>
    </v-simple-table>

  </v-card>

</template>

<script>
// import moment from 'moment';
// moment(item.timestamp+'Z').format('M D YYYY, h:mm:ss a')

export default {
  name: 'OrderBook',
  props: ['base', 'quote', 'marketStats', 'bids', 'asks', 'className'],

  components: {
  },
  computed: {
    totalBids: function () { return this.bids.slice(0,6 + 1).reverse() },
    totalAsks: function () { return this.asks.slice(0,6 + 1) },
  },
  data: () => ({
  }),
  mounted () {

  },
  methods: {

  }
};
</script>

<style scoped>
.theme--light.v-sheet {
  background-color: inherit;
  color: rgba(0, 0, 0, 0.87);
}

.itemText {
  padding-left: 0 !important;
  list-style-type: none;
}

.bookItem {
  width: 50%;
  padding-left: 0;
}

.sellText {
  font-size: small;
  padding-left: 0 !important;
  list-style-type: none;
  color: red;
}

.buyText {
  font-size: small;
  padding-left: 0 !important;
  list-style-type: none;
  color: #00FF00;
}

.price {
  border-top: 1px solid rgba(255, 255, 255, 0.3);
  border-bottom: 1px solid rgba(255, 255, 255, 0.3);
  height: 6%;
  color: white;
}

.v-data-table__wrapper {
  width: 100% !important;
}
</style>

<style>
.order-book .v-data-table--fixed-header .v-data-table__wrapper {
  overflow-y: auto;
  width: 100% !important;
}
</style>