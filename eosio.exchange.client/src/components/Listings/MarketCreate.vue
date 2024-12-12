<template>

  <v-card
    outlined
    tile
    class="pa-3"
    :class="className"
  >
    <v-row>
      <v-col cols="12" align="start" justify="center" >
        <h2 style="letter-spacing: 0.1em;">Create Market</h2>
      </v-col>
    </v-row>

    <v-row>
      <v-col
        cols="12"
        align="start"
        justify="center"
      >
        <BaseSelect @selected='setBase' tradePosition='market'/>
      </v-col>

      <v-col cols="12"
        align="center"
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
                  @click="createQuote()"
                  class="mr-4 depositButtonStyle"
                >
                  Add
                </v-btn>
                <v-btn
                  text
                  small
                  outlined
                  color="bad"
                  @click="removeQuote()"
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
import { Api, JsonRpc, /* RpcError */ } from 'eosjs';
import { JsSignatureProvider } from 'eosjs/dist/eosjs-jssig';           // development only
import swal from 'sweetalert';

import BaseSelect from '../Token/BaseSelect'

export default {
  name: 'MarketCreate',
  props: ['className'],
  components: {
    BaseSelect,
  },
  data: () => ({
    api: null,
    contract: '',
    precision: '',
    symbol: '',
    contractPlaceholder: 'fiatcryptos1',
    precisionPlaceholder: '2',
    symbolPlaceholder: 'USD',
  }),
  mounted () {
    /* EOS WALLET */
    const signatureProvider = new JsSignatureProvider([this.$store.getters.privateKey]);
    const rpc = new JsonRpc(this.$store.getters.endpoint.endpoint, { fetch });
    this.api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });
    /* EOS WALLET */

  },
  methods: {
    setBase(base) {
      this.base = base.base
      let amount = base.token.quantity.replace(/\s[A-Z]+/g, '')
      let zeros = amount.replace(/\d+\./g, '')
      this.precision = zeros.length
      this.symbol = base.token_name.toUpperCase()
      this.contract = base.token.contract
    },
    resetValidation () {
      this.$refs.form.resetValidation()
    },
    async createQuote () {
      if (this.$refs.form.validate()) {
        const quantity = parseFloat(1).toFixed(this.precision) + ' ' + this.symbol
        const contract = this.contract

        var tx_actions = [
            // createmarket
            {
              account: process.env.VUE_APP_EXCHANGE_CONTRACT,
              name: 'createmarket',
              authorization: [{
                actor: this.$store.getters.account,
                permission: 'active',
              }],
              data: {
                owner: this.$store.getters.account,
                quote: {
                  quantity: quantity,
                  contract: contract
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
            text: this.symbol + ' market created!',
          });
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
    async removeQuote () {
      if (this.$refs.form.validate()) {

        var tx_actions = [
            // removemarket
            {
              account: process.env.VUE_APP_EXCHANGE_CONTRACT,
              name: 'removemarket',
              authorization: [{
                actor: this.$store.getters.account,
                permission: 'active',
              }],
              data: {
                quote: {
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
            text: this.symbol + ' market removed!',
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
  }
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