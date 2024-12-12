<template>
  <div>
    <Nav/>
    <v-container>

      <!-- TOP HEADER -->
      <v-row>
        <v-col
          cols="8"
          align="start"
          justify="start"
          style="display: flex"
        >
          <h1 class="mb-3 PreHeader">
            SWAP
          </h1>
          <h1 class="mb-3 Header">
            {{ market.value.quantity.replace(/\d+./g, '') }} / {{ quote.symbol }}
          </h1>
        </v-col>

        <v-col
          cols="4"
          align="end"
          justify="end"
        >
            <AccountBalance
              v-bind:accountName="account"
              v-bind:market="market"
              v-bind:quote="quote"
            />
        </v-col>
      </v-row>

      <!-- CHART & ORDER BOOK -->
      <v-row>
        <v-col cols="8">
          <v-card
            outlined
            tile
            class="pa-3"
          >
            <!-- <v-btn
              disabled
              color="accent"
              class="mr-4"
              @click="selectPriceChart(true)"
            >
              <span style="color: white;">Price</span>
            </v-btn>
            <v-btn
              color="accent"
              class="mr-4"
              @click="selectPriceChart(false)"
            >
              <span style="color: white;">Depth</span>
            </v-btn> -->

            <h3 style="color: var(--v-secondary-base);">Depth</h3>

            <div
              v-if="isPriceChartSelected"
            >
              <CandleChart/>
            </div>
            <div
              v-if="!isPriceChartSelected"
            >
              <DepthChart
                v-bind:market="market"
                v-bind:base="base"
                v-bind:quote="quote"
                v-bind:bids="runningBids"
                v-bind:asks="runningAsks"
              />
            </div>
          </v-card>
        </v-col>

        <v-col cols="4">
          <PlaceSwap
            v-bind:endpoint="$store.getters.endpoint.endpoint"
            v-bind:account="account"
            v-bind:privateKey="$store.getters.privateKey"
            v-bind:market="market"
            v-bind:quote="quote"
            v-bind:marketStats="marketStats"
            className="flexFit"
          />
        </v-col>
      </v-row>

    </v-container>
  </div>
</template>

<script>
import Nav from '../Nav';
import AccountBalance from './AccountBalance';
import PlaceSwap from './PlaceSwap';
import CandleChart from '../Charts/CandleChart';
import DepthChart from '../Charts/DepthChart';

const axios = require('axios');


export default {
  name: 'Swap',
  props: [],

  components: {
    Nav,
    AccountBalance,
    PlaceSwap,
    CandleChart,
    DepthChart,
  },

  data: () => ({
    isPriceChartSelected: false,
    base: '',
    bids: [{
      id: '',
      trader: '',
      timestamp: '',
      price: {
        quantity: '',
        contract: ''
      },
      volume: {
        quantity: '',
        contract: ''
      }
    }],
    asks: [{
      id: '',
      trader: '',
      timestamp: '',
      price: {
        quantity: '',
        contract: ''
      },
      volume: {
        quantity: '',
        contract: ''
      }
    }],
    marketStats: {
      market_name: 'eoseur',
      price: {
        quantity: '1.00 EUR',
        contract: 'fiatcryptos1'
      }
    },
  }),
  computed: {
    market: function() {
      return JSON.parse(this.$route.query.market)
    },
    quote: function() {
      return JSON.parse(this.$route.query.quote)
    },
    account: function() {
      return this.$store.getters.account
    },
    runningBids: function () {
      const collector = this.bids.reduce((map, bid)=>{
          if(!map[bid.price.quantity]) {
              map[bid.price.quantity] = {
                  ...bid, 
                  volume: parseFloat(bid.volume.quantity)
              }
          } else {
              map[bid.price.quantity].volume += parseFloat(bid.volume.quantity)
          }
          return map
      }, {})

      const newBids = Object.values(collector)
      return newBids
    },
    runningAsks: function() {
      const collector = this.asks.reduce((map, ask)=>{
          if(!map[ask.price.quantity]) {
              map[ask.price.quantity] = {
                  ...ask, 
                  volume: parseFloat(ask.volume.quantity)
              }
          } else {
              map[ask.price.quantity].volume += parseFloat(ask.volume.quantity)
          }
          return map
      }, {})

      const newAsks = Object.values(collector)
      return newAsks
    }
  },
  mounted () {
    this.base = this.market.value.quantity.replace(/\d+./g, '')
    this.refresh()
  },
  methods: {
    refresh () {
      var self = this
      this.interval = setInterval(function () {
        self.getBidOrders()
        self.getAskOrders()
        self.getMarketStats()
      }, 500);
    },
    compareBids( a, b ) {
      let left = parseFloat(a.price.quantity.replace(/\s[A-Z]+/g, ''))
      let right = parseFloat(b.price.quantity.replace(/\s[A-Z]+/g, ''))

      if ( left < right ){
        return -1;
      }
      if ( left > right ){
        return 1;
      }
      return 0;
    },
    compare( a, b ) {
      let left = parseFloat(a.price.quantity.replace(/\s[A-Z]+/g, ''))
      let right = parseFloat(b.price.quantity.replace(/\s[A-Z]+/g, ''))

      if ( left < right ){
        return 1;
      }
      if ( left > right ){
        return -1;
      }
      return 0;
    },
    getBidOrders () {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: process.env.VUE_APP_EXCHANGE_CONTRACT,
        scope: this.market.key,
        table: 'bidorders',
        limit: 100,
        json: 1,
      })
      .then( (response) => {
        this.bids = response.data.rows.sort(this.compareBids)
        // console.log(this.markets)
      })
      .catch( (error) => {
        alert(error)
        // console.log(error);
      })
      .finally( () => {
        // always executed
      });
    },
    getAskOrders () {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: process.env.VUE_APP_EXCHANGE_CONTRACT,
        scope: this.market.key,
        table: 'askorders',
        limit: 100,
        json: 1,
      })
      .then( (response) => {
        this.asks = response.data.rows.sort(this.compare)
        // console.log(this.markets)
      })
      .catch( (error) => {
        alert(error)
        // console.log(error);
      })
      .finally( () => {
        // always executed
      });
    },
    getMarketStats () {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: process.env.VUE_APP_EXCHANGE_CONTRACT,
        scope: process.env.VUE_APP_EXCHANGE_CONTRACT,
        table: 'stats',
        lower_bound: this.market.key,
        limit: 1,
        json: 1,
      })
      .then( (response) => {
        this.marketStats = response.data.rows[0]
      })
      .catch( (error) => {
        alert(error)
        // console.log(error);
      })
      .finally( () => {
        // always executed
      });
    },
    selectPriceChart(isPriceChartSelected) {
      this.isPriceChartSelected = isPriceChartSelected
    }
  },
  beforeDestroy() {
    clearInterval(this.interval)
  },
};
</script>

<style scoped>
.theme--light.v-sheet {
  background-color: inherit;
  border-color: #d8d8d8;
  color: rgba(0, 0, 0, 0.87);
}
.MarketName {
  text-transform: uppercase;
}
.PreHeader {
  text-transform: uppercase;
  font-size: 2em;
  color: var(--v-primary-base);
  padding-right: 10px;
}
.Header {
  text-transform: uppercase;
  font-size: 2em;
}
.SubHeader {
  font-weight: bold;
}
.inputStyle {
  border: 1px solid #d8d8d8;
  border-radius: 5px;
  margin: 5px 12px 5px 12px;
  padding: 0px 0px 0px 5px;
  width: 15em;
  color: #9205ff;
}
::placeholder { /* Chrome, Firefox, Opera, Safari 10.1+ */
  /* color: #9205ff; */
}
input[type=text]:focus{
  outline: 1px solid #9205ff;     /* oranges! yey */
}
.v-application .success {
    background-color: #9205ff !important;
    border-color: #9205ff !important;
}

.flexFit {
  flex: 1;
  height: 100%;
}
</style>