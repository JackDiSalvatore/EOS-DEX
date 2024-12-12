<template>
  <div>

    <!-- <p>Chain ID: {{ info }} </p> -->

    <v-row>
      <v-col cols="12" align="center" justify="center" >
        <h1 style="letter-spacing: 0.2em;font-size: 3em;">Markets</h1>
      </v-col>
    </v-row>

    <v-row
      v-if="!markets[0]"
    >
      <v-col cols="12" align="center" justify="center" >
        <v-btn
          depressed
          color="accent"
          @click="gotoListings"
        >
          <span>CREATE MARKET</span>
        </v-btn>
      </v-col>
    </v-row>

    <v-row
      v-if="markets[0]"
    >
      <v-col cols="12">

      <v-card
        outlined
        tile
        class="pa-1"
      >
        <v-slide-group
          v-model="model"
          mandatory
          show-arrows
        >
          <v-slide-item
            v-for="(market, i) in markets"
            :key="i"
            v-slot:default="{ active, toggle }"
          >
            <div @click="toggle">
              <v-btn
                depressed
                :color="active ? 'primary' : 'overlay'"
                class="quoteButton"
                :class="{quoteButtonActive: active}"
                @click="getMarketPairs(market.market_name)"
              >
                <!-- <span style="color: var(--v-background-base);" -->
                {{ market.market_name }}
              </v-btn>
            </div>
          </v-slide-item>
        </v-slide-group>
      </v-card>

      </v-col>
    </v-row>

    <v-row
      v-if="marketPairs[0]"
    >
      <v-col cols="12"
        align="center"
        justify="center"
      >

        <v-card
          class="pa-3"
          outlined
          tile
        >
          <v-simple-table class="test" fixed-header>
            <template v-slot:default>
              <thead>
                  <tr>
                  <th class="text-left">TRADE</th>
                  <th class="text-left">SWAP</th>
                  <th class="text-left">LAST PRICE</th>
                  <th class="text-left">24H CHANGE</th>
                  <th class="text-left">24H HIGH</th>
                  <th class="text-left">24H LOW</th>
                  <th class="text-left">24H VOLUME</th>
                  </tr>
              </thead>
              <tbody>
                  <tr v-for="(market, i) in marketPairs" :key="i">
                  <td>
                    <!-- <a class="MarketName" @click="() => $emit('selected', market)"> -->
                    <a class="MarketName" @click="setMarket(market)">
                      {{ market.value ? market.value.quantity.replace(/\d+./g, '') : null }} / {{ quote ? quote.symbol : null }}
                    </a>
                  </td>
                  <td>
                    <v-btn
                      text
                      small
                      outlined
                      color="accent"
                      @click="gotoSwap(market)"
                      class="swapButtonStyle"
                    >
                      Swap
                    </v-btn>
                  </td>
                  <td>3.45000000</td>
                  <td class="twentyFourHourChange">+3.5%</td>
                  <td>3.75000000</td>
                  <td>3.25000000</td>
                  <td>1000000.00000000</td>
                  </tr>
              </tbody>
            </template>
          </v-simple-table>
        </v-card>

      </v-col>
    </v-row>

  </div>
</template>

<script>
const axios = require('axios');

export default {
  name: 'MarketSelect',
  props: [],
  data: () => ({
    interval: null,
    model: null,
    header: 'Select',
    info: null,
    markets: [],
    marketPairs: [],
    quote: null,
    selectedQuote: null,
    // selectedMarket: {
    //   name: ''
    // }
  }),
  mounted () {
    // this.getChain()
    this.getMarketPairs('usd')
    this.refresh()
  },
  methods: {
    refresh () {
      var self = this
      this.interval = setInterval(function () {
        self.getAllMarkets()
      }, 500);
    },
    getChain () {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_info')
      .then( (response) => {
        // alert(response)
        this.info = response.data.chain_id;
      })
      .catch( (error) => {
        alert(error)
        // console.log(error);
      })
      .finally( () => {
        // always executed
      });
    },
    getAllMarkets () {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: process.env.VUE_APP_EXCHANGE_CONTRACT,
        scope: process.env.VUE_APP_EXCHANGE_CONTRACT,
        table: 'markets',
        reverse: 1,
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
    getMarketPairs (quote) {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: process.env.VUE_APP_EXCHANGE_CONTRACT,
        scope: process.env.VUE_APP_EXCHANGE_CONTRACT,
        table: 'markets',
        lower_bound: quote,
        json: 1,
      })
      .then( (response) => {
        // this.marketPairs = response.data.rows
        if(response.data.rows[0]) {
          this.quote = {
            symbol: response.data.rows[0].quote.quantity.replace(/\d+./g, ''),
            quantity: response.data.rows[0].quote.quantity,
            contract: response.data.rows[0].quote.contract,
          }
          this.setQuote(this.quote)
        } else
          this.setQuote(null)

        if(response.data.rows[0]) {
          this.marketPairs = response.data.rows[0].bases;
        } else {
          this.marketPairs = []
        }
        // console.log(this.marketPairs)
      })
      .catch( () => {
        this.marketPairs = []
        // alert(error)
        // console.log(error);
      })
      .finally( () => {
        // always executed
      });
    },
    setMarket (market) {
      this.$emit('selected', market)
      // alert('market')
    },
    setQuote (quote) {
      this.selectedQuote = quote
      this.$emit('selectedQuote', quote)
    },
    gotoListings() {
      this.$store.dispatch('setAccountPageActive', false)
      this.$store.dispatch('setMarketPageActive', false)
      this.$store.dispatch('setListingPageActive', true)

      this.$router.push('/listings')
    },
    gotoSwap(market) {
      this.$store.dispatch('setAccountPageActive', false)
      this.$store.dispatch('setMarketPageActive', false)
      this.$store.dispatch('setListingPageActive', false)

      this.$router.push(
        {
          name: 'swap',
          query: {
            market: JSON.stringify(market),
            quote: JSON.stringify(this.selectedQuote)
          }
        }
      )
    }
  },
  beforeDestroy() {
    clearInterval(this.interval)
  },
};
</script>

<style scoped>
.MarketName {
  font-weight: bold;
  text-transform: uppercase;
  color: var(--v-primary-base);
}

.quoteButton {
  color: var(--v-secondary-base);
}

.quoteButtonActive {
  color: var(--v-background-base) !important;
  font-weight: bolder;
}

.twentyFourHourChange {
  color: var(--v-good-base);
}

.swapButtonStyle {
  border-color: var(--v-accent-base) !important;
  transition: background-color 150ms ease-in-out;
}

.swapButtonStyle:hover {
  background-color: rgba(74, 144, 226, 0.25) !important;
}

</style>