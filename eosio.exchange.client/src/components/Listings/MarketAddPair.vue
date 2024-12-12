<template>

  <v-card
    outlined
    tile
    class="pa-3"
  >
    <v-row>
      <v-col cols="12" align="start" justify="center" >
        <h2 style="letter-spacing: 0.1em;">Add Trading Pair</h2>
      </v-col>

      <v-col
        cols="6"
        align="center"
        justify="center"
      >
        <QuoteSelect @selected='setQuote' tradePosition='market'/>
      </v-col>

      <v-col
        cols="6"
        align="center"
        justify="center"
      >
        <BaseSelect @selected='setBase' tradePosition='trading pair'/>
      </v-col>

      <v-col cols="12"
        align="start"
        justify="center"
      >
        <v-form
          ref="form"
          lazy-validation
          class="ma-3"
        >
          <v-row>
            <v-col
              cols="12"
              align="end"
              justify="center"
            >
              <v-btn
                text
                small
                outlined
                color="accent"
                @click="addPair()"
                class="mr-4 depositButtonStyle"
              >
                Add
              </v-btn>
              <v-btn
                text
                small
                outlined
                color="bad"
                @click="removePair()"
                class="withdrawButtonStyle"
              >
                Remove
              </v-btn>
            </v-col>
          </v-row>
        </v-form>
      </v-col>
    </v-row>
  </v-card>

</template>

<script>
import BaseSelect from '../Token/BaseSelect'
import QuoteSelect from '../Token/QuoteSelect'

import { Api, JsonRpc, /* RpcError */ } from 'eosjs';
import { JsSignatureProvider } from 'eosjs/dist/eosjs-jssig';           // development only
import swal from 'sweetalert';

export default {
  name: 'MarketAddPair',
  props: [],
  components: {
    QuoteSelect,
    BaseSelect,
  },
  data: () => ({
    api: null,
    contract: '',
    precision: '',
    symbol: '',
    contractPlaceholder: 'eosio.token',
    precisionPlaceholder: '4',
    symbolPlaceholder: 'EOS',
    quote: null,
  }),
  mounted () {
    /* EOS WALLET */
    const signatureProvider = new JsSignatureProvider([this.$store.getters.privateKey]);
    const rpc = new JsonRpc(this.$store.getters.endpoint.endpoint, { fetch });
    this.api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });
    /* EOS WALLET */

  },
  methods: {
    setQuote(quote) {
      this.quote = quote.quote
    },
    setBase(base) {
      this.base = base.token

      let amount = base.token.quantity.replace(/\s[A-Z]+/g, '')
      let zeros = amount.replace(/\d+\./g, '')
      this.precision = zeros.length
      this.symbol = base.token_name.toUpperCase()
      this.contract = base.token.contract
    },
    resetValidation () {
      this.$refs.form.resetValidation()
    },
    async addPair () {
      if (this.$refs.form.validate()) {

        var tx_actions = [
            // addpair
            {
              account: process.env.VUE_APP_EXCHANGE_CONTRACT,
              name: 'addpair',
              authorization: [{
                actor: this.$store.getters.account,
                permission: 'active',
              }],
              data: {
                owner: this.$store.getters.account,
                quote: this.quote,
                base: {
                  quantity: parseFloat(1).toFixed(this.precision) + ' ' + this.symbol,
                  contract: this.contract
                }
              },
            }
          ]

        try {
          await this.api.transact({
            actions: tx_actions
          }, {
            blocksBehind: 3,
            expireSeconds: 30,
          });

          swal({
            text:  this.symbol + '/' + this.quote.quantity.replace(/\d+./g, '') + ' market pair created!',
          });

          this.contract = ''
          this.precision = ''
          this.symbol = ''

        } catch (e) {
          let s = e.toString().replace('Error: ','').replace('assertion failure with message: ', '')
          swal({
            icon: "error",
            text: s.charAt(0).toUpperCase() + s.slice(1),
          });
        }

        this.resetValidation()
      }
    },
    async removePair () {
      if (this.$refs.form.validate()) {

        var tx_actions = [
            // removepair
            {
              account: process.env.VUE_APP_EXCHANGE_CONTRACT,
              name: 'removepair',
              authorization: [{
                actor: this.$store.getters.account,
                permission: 'active',
              }],
              data: {
                quote: this.quote,
                base: {
                  quantity: parseFloat(1).toFixed(this.precision) + ' ' + this.symbol,
                  contract: this.contract
                }
              },
            }
          ]

        try {
          await this.api.transact({
            actions: tx_actions
          }, {
            blocksBehind: 3,
            expireSeconds: 30,
          });

          swal({
            text: this.symbol + '/' + this.quote.quantity.replace(/\d+./g, '') + ' market pair removed!',
          });

          this.contract = ''
          this.precision = ''
          this.symbol = ''

        } catch (e) {
          let s = e.toString().replace('Error: ','').replace('assertion failure with message: ', '')
          swal({
            icon: "error",
            text: s.charAt(0).toUpperCase() + s.slice(1),
          });
        }

        this.resetValidation()
      }
    },
  },
};
</script>

<style scoped>
.v-input--selection-controls {
    margin-top: 0 !important;
    padding-top: 0 !important;
}

input {
  background-color: transparent;
  border: 0.5px solid #469FE2 !important;
  border-radius: 5px;
  text-align: right;
  font-weight: 500;
  /* float: right; */
  padding-right: 5px;
  width: 100%;
}

::placeholder {
  color: #d8d8d8;
  opacity: 0.5; /* Firefox */
}

.contractInput {
  color: var(--v-primary-base);
  font-weight: bold;
}

/* Button Style */
.withdrawButtonStyle {
  border-color: rgba(255,0,0,0.8) !important;
  transition: background-color 150ms ease-in-out;
}

.withdrawButtonStyle:hover {
  background-color: rgba(255, 0, 0, 0.25) !important;
}

.depositButtonStyle {
  border-color: var(--v-accent-base) !important;
  transition: background-color 150ms ease-in-out;
}

.depositButtonStyle:hover {
  background-color: rgba(74, 144, 226, 0.25) !important;
}
/************/

</style>