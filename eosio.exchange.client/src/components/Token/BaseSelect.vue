<template>

  <v-menu offset-y>
    <template v-slot:activator="{ on }">
      <v-btn
        class="baseSelectorButton"
        depressed
        dark
        v-on="on"
      >
        <span class="baseQuantity">{{ selectedBase ? selectedBase.token_name : tradePosition }}</span>
        <span class="baseContract">{{ selectedBase ? selectedBase.token.contract : null }}</span>
      </v-btn>
    </template>
    <v-list>
      <v-list-item
        v-for="(market, index) in markets"
        :key="index"
        @click="setBase(market)"
      >
        <v-list-item-title style="text-align: center;">
          <span class="baseQuantity">{{ market.token_name.toUpperCase() }}</span>
          <span class="baseContract">{{ market.token.contract.toLowerCase() }}</span>
        </v-list-item-title>
      </v-list-item>
    </v-list>
  </v-menu>

</template>


<script>
const axios = require('axios');

export default {
  name: 'BaseSelect',
  props: ['tradePosition'],

  data: () => ({
    markets: [],
    selectedBase: null,
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
        table: 'tokens',
        limit: 1000,
        json: 1,
      })
      .then( (response) => {
        this.markets = response.data.rows
        // alert(this.market)
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
    setBase(market) {
      this.selectedBase = market
      this.$emit('selected', market)
    }
  },
  beforeDestroy() {
    clearInterval(this.interval)
  },
};
</script>

<style scoped>
.baseSelectorButton {
  background-color: rgba(74, 144, 226, 0.25) !important;
  border: 1px solid var(--v-accent-base);
  width: 100%;
}

.baseContract {
  font-weight: bolder;
  font-size: 0.85rem;
  color: var(--v-primary-base);
  padding-right: 5px;
  margin-left: 20px;
  text-transform: lowercase;
}

.baseQuantity {
  color: var(--v-secondary-base);
  padding-left: 5px;
  font-weight: bolder;
  font-size: 1rem;
}
</style>