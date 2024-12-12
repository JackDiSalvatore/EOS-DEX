<template>

  <v-container ma-0 pa-0>

    <v-row>

      <!-- FORM TO ADD TOKENS -->
      <v-col
        cols="12"
        offset="0"
        align="center"
        justify="center"
      >
        <v-card
          outlined
          tile
        >
          <v-row>
            <v-col
              cols="12"
              align="center"
              justify="center"
            >
              <h2 style="letter-spacing: 0.1em;">Supported Tokens</h2>
            </v-col>
          </v-row>

          <v-form
            ref="form"
            lazy-validation
            class="ma-3"
          >
            <v-row class="mx-12">
              <v-col
                cols="6"
                offset="0"
                align="center"
                justify="center"
              >
                <input
                  class="myInput"
                  :class="{contractInput: contract}"
                  type="text"
                  v-model="contract"
                  placeholder="Contract Name: eosio.token"
                  required
                >
              </v-col>
              <v-col
                cols="2"
                offset="0"
              >
                <input
                  class="myInput"
                  type="text"
                  v-model="precision"
                  label="Amount"
                  placeholder="Precision: 4"
                  required
                >
              </v-col>
              <v-col
                cols="2"
                offset="0"
              >
                <input
                  class="myInput"
                  type="text"
                  v-model="symbol"
                  label="Amount"
                  placeholder="Symbol: EOS"
                  required
                >
              </v-col>
              <v-col
                cols="2"
                offset="0"
              >
                <v-btn
                  text
                  small
                  outlined
                  color="accent"
                  @click="addToken()"
                  class="depositButtonStyle"
                >
                  Add
                </v-btn>
              </v-col>
            </v-row>
          </v-form>
        </v-card>
      </v-col>


      <!-- TABLE TO SEE AND REMOVE TOKENS -->
      <v-col
        cols="12"
        offset="0"
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
                    <th class="text-left">Token</th>
                    <th class="text-left">Contract</th>
                    <th class="text-left">Remove Support</th>
                  </tr>
              </thead>
              <tbody>
                  <tr v-for="(token, i) in tokens" :key="i">
                    <td>
                      <span style="text-transform: uppercase;">{{ token.token_name }}</span>
                    </td>
                    <td>
                      <span style="text-transform: lowercase;color: var(--v-primary-base);font-weight: bold;">
                        {{ token.token.contract }}
                      </span>
                    </td>
                    <td>
                      <v-btn
                        text
                        small
                        outlined
                        color="bad"
                        @click="removeToken(token)"
                        class="withdrawButtonStyle"
                      >
                        REMOVE
                      </v-btn>
                    </td>
                  </tr>
              </tbody>
            </template>
          </v-simple-table>
        </v-card>
      </v-col>
    </v-row>

  </v-container>

</template>

<script>
import { Api, JsonRpc, /* RpcError */ } from 'eosjs';
import { JsSignatureProvider } from 'eosjs/dist/eosjs-jssig';           // development only
import swal from 'sweetalert';

const axios = require('axios');

export default {
  name: 'TokenAddRemove',

  components: {

  },
  data: () => ({
    contract: null,
    precision: null,
    symbol: null,
    tokens: null,
  }),
  mounted () {
    /* EOS WALLET */
    const signatureProvider = new JsSignatureProvider([this.$store.getters.privateKey]);
    const rpc = new JsonRpc(this.$store.getters.endpoint.endpoint, { fetch });
    this.api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });

    this.refresh()
  },
  methods: {
    refresh () {
      var self = this
      this.interval = setInterval(function () {
        self.getTokens()
      }, 500);
    },
    resetValidation () {
      this.$refs.form.resetValidation()
    },
    getTokens() {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: process.env.VUE_APP_EXCHANGE_CONTRACT,
        scope: process.env.VUE_APP_EXCHANGE_CONTRACT,
        table: 'tokens',
        limit: 1000,
        json: 1,
      })
      .then( (response) => {
        this.tokens = response.data.rows
      })
      .catch( (error) => {
        alert(error)
        // console.log(error);
      })
      .finally( () => {
        // always executed
      });
    },
    async addToken() {
      if (this.$refs.form.validate()) {
        var tx_actions = [
            // addtoken
            {
              account: process.env.VUE_APP_EXCHANGE_CONTRACT,
              name: 'addtoken',
              authorization: [{
                actor: this.$store.getters.account,
                permission: 'active',
              }],
              data: {
                owner: this.$store.getters.account,
                token: {
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
            text: this.symbol + ' token added!',
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
    async removeToken(token) {
      if (this.$refs.form.validate()) {
        var tx_actions = [
            // removetoken
            {
              account: process.env.VUE_APP_EXCHANGE_CONTRACT,
              name: 'removetoken',
              authorization: [{
                actor: this.$store.getters.account,
                permission: 'active',
              }],
              data: {
                token: {
                  quantity: token.token.quantity,
                  contract: token.token.contract
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
            text: token.token_name.toUpperCase() + ' token removed!',
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
    }
  },
  beforeDestroy() {
    clearInterval(this.interval)
  },
};
</script>

<style scoped>


/* Input Style */
input.contractInput {
  color: var(--v-primary-base);
  font-weight: bold;
  border-bottom: 0.5px solid var(--v-accent-base) !important;
  width: 100% !important;
}

input.myInput {
  text-align: right;
  border: 0.5px solid var(--v-accent-base) !important;
  border-radius: 5px;
  padding: 0px 5px 0px 5px !important;
  width: 100% !important;
}

input.myInput:focus {
  outline: none;
}

::placeholder {
  color: #d8d8d8;
  opacity: 0.5; /* Firefox */
}

/* Button Style */
.withdrawButtonStyle {
  border-color: rgba(255,0,0,0.8) !important;
  transition: background-color 150ms ease-in-out;
  /* width: 100% !important; */
}

.withdrawButtonStyle:hover {
  background-color: rgba(255, 0, 0, 0.25) !important;
}

.depositButtonStyle {
  border-color: var(--v-accent-base) !important;
  transition: background-color 150ms ease-in-out;
  width: 100% !important;
}

.depositButtonStyle:hover {
  background-color: rgba(74, 144, 226, 0.25) !important;
}
/************/
</style>