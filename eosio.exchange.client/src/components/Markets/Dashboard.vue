<template>

  <div>

    <Nav/>

    <v-container>

      <v-row>

        <!-- Market Picker -->
        <v-col
          cols="8" offset="2"
          v-if="!selectedMarket.key"
        >
          <MarketSelect
            @selected='setMarket'
            @selectedQuote='setQuote'
          />
        </v-col>

      </v-row>

    </v-container>

  </div>

</template>

<script>
import MarketSelect from './MarketSelect';
import Nav from '../Nav';

export default {
  name: 'Dashboard',

  components: {
    MarketSelect,
    Nav,
  },
  data: () => ({
    account: null,
    privateKey: null,
    header: 'Dashboard',
    selectedMarket: {
    },
    selectedQuote: {
    },
    // styles
    alignmentsAvailable: [
      'start',
      'center',
      'end',
      'baseline',
      'stretch',
    ],
    dense: false,
    justifyAvailable: [
      'start',
      'center',
      'end',
      'space-around',
      'space-between',
    ],
  }),
  mounted () {

  },
  methods: {
    setMarket(market) {
      this.selectedMarket  = market
      this.$store.dispatch('setMarketPageActive', false)

      this.$router.push(
        {
          name: 'market',
          query: {
            market: JSON.stringify(this.selectedMarket),
            quote: JSON.stringify(this.selectedQuote)
          }
        }
      )
    },
    setQuote(quote) {
      this.selectedQuote = quote
      // alert(JSON.stringify(this.selectedMarket, 2, null))
    }
  }
};
</script>

<style scoped>
.flexFit {
  flex: 1;
  height: 100%;;
}
</style>