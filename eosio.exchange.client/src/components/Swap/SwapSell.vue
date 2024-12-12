<template>
  <v-form
    ref="form"
    lazy-validation
    class="ma-3"
  >

    <v-row>
      <v-col
        cols="3" small="7"
        align="end"
        justify="center"
        class="pr-0"
      >
        Price:
      </v-col>
      <v-col
        cols="7" small="3"
        align="end"
        justify="center"
      >
        <span style="color: white;">{{ price }}</span>
      </v-col>

      <v-col
        cols="2"
        align="start"
        justify="center"
        class="pl-0"
      >
        {{ priceLabel.symbol }}
      </v-col>
    </v-row>

    <v-row>
      <v-col
        cols="3" small="7"
        align="end"
        justify="center"
        class="pr-0"
      >
        Amount:
      </v-col>
      <v-col
        cols="7" small="3"
      >
        <input
          type="text"
          v-model="order.amount"
          label="Amount"
          :placeholder="amountPlaceholder"
          required
        >
      </v-col>
      <v-col
        cols="2"
        align="start"
        justify="center"
        class="pl-0"
      >
        {{ amountLabel.replace(/\d+./g, '') }}
      </v-col>
    </v-row>

    <v-row>
      <v-col
        cols="3"
        align="end"
        justify="center"
        class="pr-0"
      >
        Total:
      </v-col>
      <v-col
        cols="7"
        align="end"
        justify="center"
        class=""
      >
        <!-- {{ parseFloat( Math.ceil(price * order.amount * 100)/100 ).toFixed(2) }} -->
        {{ order.amount ? order.amount : amountLabel.replace(/\s[A-Z]+/g, '') }}
      </v-col>
      <v-col
        cols="2"
        align="start"
        justify="center"
        class="pl-0"
      >
        {{ amountLabel.replace(/\d+./g, '') }}
      </v-col>
    </v-row>

    <v-row>
      <v-col
        cols="12"
        align="end"
        justify="center"
      >
        <v-btn
          depressed
          color="bad"
          class="mr-4"
          @click="sellOrder"
        >
          <span class="sellButtonText">Sell</span>
        </v-btn>
      </v-col>
    </v-row>

  </v-form>

</template>

<script>
import { Api, JsonRpc, /* RpcError */ } from 'eosjs';
import { JsSignatureProvider } from 'eosjs/dist/eosjs-jssig';           // development only
import swal from 'sweetalert';


export default {
  name: 'SwapSell',
  props: ['endpoint','account', 'privateKey', 'market', 'quote', 'price'],
  components: {
  },
  data: () => ({
    api: null,
    // checkbox: false,
    // checkbox: false,
    priceLabel: '',
    pricePlaceholder: '',
    amountLabel: '',
    amountPlaceholder: '',
    order: {
      price: '',
      amount: '',
    },
  }),
  mounted () {
    /* EOS WALLET */
    const signatureProvider = new JsSignatureProvider([this.privateKey]);
    const rpc = new JsonRpc(this.endpoint, { fetch });
    this.api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });
    /* EOS WALLET */

    this.priceLabel = this.quote
    this.amountLabel = this.market.value.quantity

    this.pricePlaceholder = this.quote.quantity.replace(/\s[A-Z]+/g, '')
    this.amountPlaceholder = this.market.value.quantity.replace(/\s[A-Z]+/g, '')
  },
  methods: {
    resetValidation () {
      this.$refs.form.resetValidation()
    },
    async sellOrder () {
      if (this.$refs.form.validate()) {

        var tx_actions = [
          // deposit
          {
            account: this.market.value.contract,
            name: 'transfer',
            authorization: [{
              actor: this.account,
              permission: 'active',
            }],
            data: {
              from: this.account,
              to: process.env.VUE_APP_EXCHANGE_CONTRACT,
              quantity: this.order.amount + ' ' + this.amountLabel.replace(/\d+./g, ''),
              memo: 'deposit'
            },
          },
          // place trade
          {
            account: process.env.VUE_APP_EXCHANGE_CONTRACT,
            name: 'trade',
            authorization: [{
              actor: this.account,
              permission: 'active',
            }],
            data: {
              trader: this.account,
              order_type: false,
              price: {
                quantity: this.price + ' ' + this.priceLabel.symbol,
                contract: this.quote.contract
              },
              volume:{
                quantity: this.order.amount + ' ' + this.amountLabel.replace(/\d+./g, ''),
                contract: this.market.value.contract
              },
              auto_withdraw: true
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
          this.order.amount = ''
        } catch (e) {
          let s = e.toString().replace('Error: ','').replace('assertion failure with message: ', '')
          swal({
            icon: "error",
            text: s.charAt(0).toUpperCase() + s.slice(1),
          });
          // alert('\nCaught exception: ' + e);
          // if (e instanceof RpcError)
            // alert(JSON.stringify(e.json, null, 2));
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
  /* float: right; */
  padding-right: 5px;
  width: 100%;
}

.sellButtonText {
  letter-spacing: 0.2em;
  color: var(--v-white--base);
}

::placeholder {
  color: #d8d8d8;
  opacity: 0.5; /* Firefox */
}
</style>