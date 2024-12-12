<template>

  <v-card outlined tile class="px-3">

    <v-row>
      <v-col cols="4" align="start" justify="center">
        <h3 class="accountName">Exchange</h3>
      </v-col>

      <v-col cols="8">
        <div style="display: flex;">
          <div style="padding: 0px;width: 60%;">{{ userQuoteBalance }}</div>
          <div class="text-right nameLabel" style="padding: 0px;width: 40%;">{{ quote.contract }}</div>
        </div>

        <div style="display: flex;">
          <div style="padding: 0px;width: 60%;">{{ userBaseBalance }}</div>
          <div class="text-right nameLabel" style="padding: 0px;width: 40%;">{{ market.value.contract }}</div>
        </div>
      </v-col>
    </v-row>

  </v-card>

</template>

<script>
const axios = require('axios');

export default {
  name: 'ExchangeBalance',
  props: ['accountName', 'market', 'quote'],
  data: () => ({
    api: null,
    header: 'Balance',
    interval: null,
    userQuoteBalance: null,
    userBaseBalance: null,
    exAccountTable: process.env.VUE_APP_EXCHANGE_CONTRACT,
    quoteAccounts: [{
      balance: ''
    }],
    baseAccounts: [{
      balance: ''
    }],
    // requested_token: ''
  }),
  mounted () {

    this.refresh()
  },
  methods: {
    refresh () {
      var self = this
      this.interval = setInterval(function () {
        self.getBaseBalance(self.accountName)
        self.getQuoteBalance(self.accountName)
      }, 1000);
    },
    getQuoteBalance (user_account) {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: process.env.VUE_APP_EXCHANGE_CONTRACT,
        scope: user_account,
        table: 'exaccounts',
        json: 1,
      })
      .then( (response) => {
        this.quoteAccounts = response.data.rows;
        this.userQuoteBalance = this.quote.quantity

        for(let i = 0; i < this.quoteAccounts.length; i++) {
          if(this.quoteAccounts[i].balance.quantity.replace(/\d+./g, '') === this.quote.symbol) {
            this.userQuoteBalance = this.quoteAccounts[i].balance.quantity
          }
        }
      })
      .catch( (error) => {
        alert(error)
        // console.log(error);
      })
      .finally( () => {
        // always executed
      });
    },
    getBaseBalance (user_account) {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: process.env.VUE_APP_EXCHANGE_CONTRACT,
        scope: user_account,
        table: 'exaccounts',
        json: 1,
      })
      .then( (response) => {
        this.baseAccounts = response.data.rows
        this.userBaseBalance = this.market.value.quantity

        for(let i = 0; i < this.baseAccounts.length; i++) {
          if(this.baseAccounts[i].balance.quantity.replace(/\d+./g, '') === this.market.value.quantity.replace(/\d+./g, '')) {
            this.userBaseBalance = this.baseAccounts[i].balance.quantity
          }
        }
      })
      .catch( (error) => {
        alert(error)
        // console.log(error);
      })
      .finally( () => {
        // always executed
      });
    },
  },
  beforeDestroy() {
    clearInterval(this.interval)
  },
};
</script>

<style scoped>
.accountName {
  text-transform: capitalize;
  letter-spacing: 0.05em;
  /* color: var(--v-primary-base); */
}

.nameLabel {
  font-weight: bold;
  color: var(--v-primary-base);
}

.balance {
  text-transform: capitalize;
  letter-spacing: 0.05em;
  font-weight: lighter;
  color: var(--v-secondary-base);
}
</style>