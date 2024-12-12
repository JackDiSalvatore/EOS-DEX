<template>

  <div>

    <Nav/>

    <v-container>

      <v-row>

        <v-col
          cols="6"
          align="center"
          justify="center"
        > 
          <MarketCreate className="flexFit"/>
        </v-col>
        <v-col
          cols="6"
          align="center"
          justify="center"
        >
          <MarketAddPair className="flexFit"/>
        </v-col>

      </v-row>

      <v-row>

        <v-col
          cols="12"
          offset="0"
          align="start"
          justify="center"
        >
          <TokenAddRemove/>
        </v-col>

      </v-row>

    </v-container>

  </div>

</template>

<script>
import Nav from '../Nav';
import MarketCreate from './MarketCreate'
import MarketAddPair from './MarketAddPair'
import TokenAddRemove from './TokenAddRemove'

export default {
  name: 'Listings',

  components: {
    Nav,
    MarketCreate,
    MarketAddPair,
    TokenAddRemove
  },
  data: () => ({
    account: null,
    privateKey: null,
    header: 'Listings',
    selectedMarket: {
    },
    selectedQuote: {
    },
  }),
  mounted () {

  },
  methods: {

  }
};
</script>

<style scoped>
.flexFit {
  flex: 1 !important;
  height: 100% !important;
}
</style>