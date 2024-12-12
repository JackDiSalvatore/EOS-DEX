<template>

  <div>
    <Nav/>

    <v-container>

      <v-row>
        <v-col cols="12" align="start" justify="center">
          <h1 style="text-transform: uppercase;letter-spacing: 0.1em;">{{ account }}</h1>
        </v-col>
      </v-row>

      <v-card outlined tile class="flexFit pa-4 mb-3">
        <v-row>
          <v-col cols="12">
            <h3>Deposit</h3>
          </v-col>
        </v-row>

        <v-form
          ref="form"
          lazy-validation
        >
          <v-row>
            <v-col cols="3"
              style="padding-top: 0;"
            >
              <input
                type="text"
                v-model="depositAmount"
                label="Amount"
                placeholder="Amount: "
                required
                style="height: 100%;"
              >
            </v-col>

            <v-col
              cols="3"
              style="padding-top: 0;"
            >
              <BaseSelect @selected='setDeposit' tradePosition='Token'/>
            </v-col>

            <v-col cols="6"
              style="padding-top: 0;"
            >
              <v-btn
                text
                small
                outlined
                color="accent"
                @click="deposit()"
                class="depositButtonStyle"
              >
                deposit
              </v-btn>
            </v-col>
          </v-row>
        </v-form>

        <v-row>
          <v-col cols="12">
            <h3>Withdraw</h3>
          </v-col>
        </v-row>

        <v-form
          ref="form"
          lazy-validation
        >
          <v-row>
            <v-col cols="3"
              style="padding-top: 0;"
            >
              <input
                type="text"
                v-model="withdrawAmount"
                label="Amount"
                placeholder="Amount: "
                required
                style="height: 100%;"
              >
            </v-col>

            <v-col
              cols="3"
              style="padding-top: 0;"
            >
              <BaseSelect @selected='setWithdraw' tradePosition='Token'/>
            </v-col>

            <v-col cols="6"
              style="padding-top: 0;"
            >
              <v-btn
                text
                small
                outlined
                color="error"
                @click="withdraw"
                class="withdrawButtonStyle"
              >
                withdraw
              </v-btn>
            </v-col>
          </v-row>
        </v-form>
      </v-card>

      <v-row>
        <v-col cols="6">
          <v-card outlined tile class="pa-6">
            <h3>Exchange Balance</h3>
            <v-simple-table dense height="180px">
              <template v-slot:default>
                <thead>
                  <tr>
                    <th class="text-left">Balance</th>
                    <th class="text-left">Contract</th>
                  </tr>
                </thead>
                <tbody>
                  <tr v-for="item in exaccounts" :key="item.account_id">
                    <td class="tableData quantityStyle">{{ item.balance.quantity }}</td>
                    <td class="tableData contractName">{{ item.balance.contract }}</td>
                  </tr>
                </tbody>
              </template>
            </v-simple-table>
          </v-card>
        </v-col>

        <v-col cols="6">
          <v-card outlined tile class="pa-6">
          <h3>Account Balance</h3>
          <v-simple-table dense height="180px">
            <template v-slot:default>
              <thead>
                <tr>
                  <th class="text-left">Balance</th>
                  <th class="text-left">Contract</th>
                </tr>
              </thead>
              <tbody v-for="account in accounts" :key="account.contract">
                <tr v-for="quantity in account.quantity" :key="quantity.balance">
                  <td class="tableData quantityStyle">{{ quantity.balance }}</td>
                  <td class="tableData contractName">{{ account.contract }}</td>
                </tr>
              </tbody>
            </template>
          </v-simple-table>
          </v-card>
        </v-col>
      </v-row>

    </v-container>

  </div>

</template>

<script>
const axios = require('axios');
import { Api, JsonRpc, /* RpcError */ } from 'eosjs';
import { JsSignatureProvider } from 'eosjs/dist/eosjs-jssig';           // development only
import swal from 'sweetalert';

import Nav from '../Nav'
import BaseSelect from '../Token/BaseSelect'

export default {
  name: 'AccountInfo',
  props: [],
  components: {
    Nav,
    BaseSelect,
  },
  data: () => ({
    api: null,
    interval: null,
    tokens: null,
    exaccounts: null,
    token_contracts: [],
    accounts: new Array(),
    withdrawToken: null,
    withdrawAmount: null,
    depositToken: null,
    depositAmount: null,
  }),
  mounted () {
    /* EOS WALLET */
    const signatureProvider = new JsSignatureProvider([this.$store.getters.privateKey]);
    const rpc = new JsonRpc(this.$store.getters.endpoint.endpoint, { fetch });
    this.api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });
    /* EOS WALLET */

    this.getExchangeBalance(process.env.VUE_APP_EXCHANGE_CONTRACT, self.account)
    this.refresh()
  },
  computed: {
    account: function() {
      return this.$store.getters.account
    },
  },
  methods: {
    refresh () {
      var self = this
      this.interval = setInterval(function () {
        self.getExchangeTokens()
        self.getExchangeBalance(process.env.VUE_APP_EXCHANGE_CONTRACT, self.account)
        self.getAllTokenBalances()
      }, 1000);
    },
    setDeposit(base) {
      this.depositToken = base
    },
    setWithdraw(base) {
      this.withdrawToken = base
    },
    getTokenBalance (account) {
      return axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: account.contract, //this.token_contracts[index],
        scope: this.account,
        table: 'accounts',
        json: 1,
      })
      // .then( (response) => {
      //   // this.accounts[index].quantity = response.data.rows
      //   // this.accounts[index] = {...this.accounts[index], quantity: response.data.rows}
      // })
      // .catch( (error) => {
      //   alert(error)
      //   // console.log(error);
      // })
      // .finally( () => {
      //   // always executed
      // });
    },
    async getAllTokenBalances() {
      if(this.accounts) {
        const promises = this.accounts.map((account) => this.getTokenBalance(account).then( (response) => {
          return {
            ...account,
            quantity: response.data.rows
          }
        }).catch(e => {
          alert(e)
          return account
        }))

        const results = await Promise.all(promises)
        // alert(JSON.stringify(results))
        this.accounts = results
      }
    },
    // Get existing exchange supported tokens
    getExchangeTokens() {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: process.env.VUE_APP_EXCHANGE_CONTRACT,
        scope: process.env.VUE_APP_EXCHANGE_CONTRACT,
        table: 'tokens',
        limit: 1000,
        json: 1,
      })
      .then( (response) => {
        this.tokens = response.data.rows

        for(let i = 0; i < this.tokens.length; i++) {
          const contract_name = this.tokens[i].token.contract
          this.token_contracts.indexOf(contract_name) === -1 ? this.token_contracts.push(contract_name) : null;

          if(this.accounts)
            this.accounts.map(function(e) { return e.contract }).indexOf(contract_name) === -1 ? this.accounts.push({ contract: contract_name, quantity: [] }) : null
        }
      })
      .catch( (error) => {
        alert(error)
      })
      .finally( () => {
      });
    },
    getExchangeBalance (contract_account, user_account) {
      axios.post(this.$store.getters.endpoint.endpoint + '/v1/chain/get_table_rows', {
        code: contract_account,
        scope: user_account,
        table: 'exaccounts',
        json: 1,
      })
      .then( (response) => {
        this.exaccounts = response.data.rows
      })
      .catch( (error) => {
        alert(error)
        // console.log(error);
      })
      .finally( () => {
        // always executed
      });
    },
    // EOSIO Actions
    async deposit() {
      const depositAmount = this.depositAmount + ' '  + this.depositToken.token_name.toUpperCase()

      try {
        await this.api.transact({
          actions: [{
            account: this.depositToken.token.contract,
            name: 'transfer',
            authorization: [{
              actor: this.account,
              permission: 'active',
            }],
            data: {
              from: this.account,
              to: process.env.VUE_APP_EXCHANGE_CONTRACT,
              quantity: depositAmount,
              memo: 'deposit'
            },
          }]
        }, {
          blocksBehind: 3,
          expireSeconds: 30,
        });

        swal({
          text: 'Successfully deposited ' + depositAmount
        });

      } catch (e) {
        let s = e.toString().replace('Error: assertion failure with message: ','')
        swal({
          icon: "warning",
          text: s.charAt(0).toUpperCase() + s.slice(1),
        });
        // alert('\nCaught exception: ' + e);
        // if (e instanceof RpcError)
          // alert(JSON.stringify(e.json, null, 2));
      }
    },
    async withdraw () {
      const withdrawAmount = this.withdrawAmount + ' '  + this.withdrawToken.token_name.toUpperCase()

      try {
        await this.api.transact({
          actions: [{
            account: process.env.VUE_APP_EXCHANGE_CONTRACT,
            name: 'withdraw',
            authorization: [{
              actor: this.account,
              permission: 'active',
            }],
            data: {
              from: this.account,
              token: {
                quantity: withdrawAmount,
                contract: this.withdrawToken.token.contract
              },
            },
          }]
        }, {
          blocksBehind: 3,
          expireSeconds: 30,
        });

        swal({
          text: 'Successfully withdrew ' + this.withdrawAmount
        });

      } catch (e) {
        let s = e.toString().replace('Error: assertion failure with message: ','')
        swal({
          icon: "warning",
          text: s.charAt(0).toUpperCase() + s.slice(1),
        });
        // alert('\nCaught exception: ' + e);
        // if (e instanceof RpcError)
          // alert(JSON.stringify(e.json, null, 2));
      }
    },
  },
  beforeDestroy() {
    clearInterval(this.interval)
  },
};
</script>

<style scoped>
.flexFit {
  flex: 1;
  height: 100%;
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

.tableData {
  border-bottom: 1px solid rgba(255, 255, 255, 0.12);
  width: 33%;
}

.quantityStyle {
  font-weight: bold;
  /* color: var(--v-accent-base) !important; */
}

.contractName {
  font-weight: bold;
  color: var(--v-primary-base) !important;
}

.withdrawButtonStyle {
  border-color: rgba(255,0,0,0.8) !important;
  transition: background-color 150ms ease-in-out;
}

.withdrawButtonStyle:hover {
  background-color: rgba(255, 0, 0, 0.25) !important;
  /* color: var(--v-background-base) !important; */
  /* font-weight: bolder !important; */
}

.depositButtonStyle {
  border-color: var(--v-accent-base) !important;
  transition: background-color 150ms ease-in-out;
}

.depositButtonStyle:hover {
  background-color: rgba(74, 144, 226, 0.25) !important;
}

/* INPUT */
.v-input--selection-controls {
  margin-top: 0 !important;
  padding-top: 0 !important;
}

input {
  background-color: transparent;
  border: 0.5px solid #469FE2 !important;
  border-radius: 5px;
  text-align: right;
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
</style>