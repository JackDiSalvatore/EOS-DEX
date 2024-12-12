<template>

  <v-menu offset-y>
    <template v-slot:activator="{ on }">
      <v-btn
        class="quoteSelectorButton"
        depressed
        dark
        v-on="on"
      >
        <span class="quoteQuantity">{{ selectedQuote ? selectedQuote.quantity.replace(/\d+./g, '') : tradePosition }}</span>
        <span class="quoteContract">{{ selectedQuote ? selectedQuote.contract : null }}</span>
      </v-btn>
    </template>
    <v-list>
      <v-list-item
        v-for="(market, index) in markets"
        :key="index"
        @click="setQuote(market)"
      >
        <v-list-item-title style="text-align: center;">
          <span class="quoteQuantity">{{ market.quote.quantity.replace(/[\d+\.0 ]/g,' ') }}</span>
          <span class="quoteContract">{{ market.quote.contract }}</span>
        </v-list-item-title>
      </v-list-item>
    </v-list>
  </v-menu>

</template>


<script>
const axios = require('axios');

export default {
  name: 'QuoteSelect',
  props: ['tradePosition'],

  data: () => ({
    markets: [],
    selectedQuote: null,
  }),
  mounted () {
    this.refresh()
  },
  methods: {
    refresh () {
      var self = this
      this.interval = setInterval(function () {
        self.getAllMarkets()
      }, 500);
    },
    getAllMarkets () {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: process.env.VUE_APP_EXCHANGE_CONTRACT,
        scope: process.env.VUE_APP_EXCHANGE_CONTRACT,
        table: 'markets',
        json: 1,
      })
      .then( (response) => {
        this.markets = response.data.rows
      })
      .catch( () => {
        this.markets = []
        // alert(error)
        // console.log(error);
      })
      .finally( () => {
        // always executed
      });
    },
    setQuote(market) {
      this.selectedQuote = market.quote
      this.$emit('selected', market)
    }
  },
  beforeDestroy() {
    clearInterval(this.interval)
  },
};
</script>

<style scoped>
.quoteSelectorButton {
  background-color: rgba(74, 144, 226, 0.25) !important;
  border: 1px solid var(--v-accent-base);
  width: 100%;
}

.quoteContract {
  font-weight: bolder;
  font-size: 0.85rem;
  color: var(--v-primary-base);
  margin-left: 20px;
  text-transform: lowercase;
}

.quoteQuantity {
  font-weight: bolder;
  font-size: 1rem;
  color: var(--v-secondary-base);
}
</style>