<template>

  <div v-if="accountAsks[0]">
    <v-simple-table dense height="180px">
      <template v-slot:default>
        <thead>
            <tr>
            <th class="text-left">ID</th>
            <th class="text-left">Time</th>
            <!-- <th class="text-left">Trader</th> -->
            <th class="text-left">Price ({{ quote.symbol }})</th>
            <th class="text-left">Volume ({{ base }})</th>
            <th class="text-left">Cancel</th>
            </tr>
        </thead>
        <tbody>
            <tr v-for="item in accountAsks" :key="item.id">
            <td>{{ item.id }}</td>
            <td>{{ item.timestamp }}</td>
            <!-- <td>{{ item.trader }}</td> -->
            <td>{{ item.price.quantity.replace(/\s[A-Z]+/g, '') }}</td>
            <td>{{ item.volume.quantity.replace(/\s[A-Z]+/g, '') }}</td>
            <td>
              <v-btn
                text
                small
                outlined
                color="error"
                @click="cancel(item.id,true)"
                class="buttonStyle"
              >
                cancel
              </v-btn>
            </td>
            </tr>
        </tbody>
      </template>
    </v-simple-table>
  </div>

</template>

<script>
import { Api, JsonRpc, /* RpcError */ } from 'eosjs';
import { JsSignatureProvider } from 'eosjs/dist/eosjs-jssig';           // development only
import swal from 'sweetalert';

export default {
  name: 'OpenBuyOrders',
  props: ['endpoint', 'privateKey', 'market', 'base', 'quote', 'account', 'bids', 'asks'],

  components: {
  },

  data: () => ({
    api: null
  }),
  mounted () {
    /* EOS WALLET */
    const signatureProvider = new JsSignatureProvider([this.privateKey]);
    const rpc = new JsonRpc(this.endpoint, { fetch });
    this.api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });
    /* EOS WALLET */
  },
  computed: {
    accountAsks: function () { return this.asks.filter(row => row.trader == this.account) },
  },
  watch: {

  },
  methods: {
    // EOSIO Actions
    async cancel (id, order_type) {
      let quote = {
                quantity: this.quote.quantity,
                contract: this.quote.contract
              }

      try {
        await this.api.transact({
          actions: [{
            account: process.env.VUE_APP_EXCHANGE_CONTRACT,
            name: 'cancel',
            authorization: [{
              actor: this.account,
              permission: 'active',
            }],
            data: {
              base: this.market.value,
              quote: quote,
              trader: this.account,
              order_type: order_type,  // 0 = BID, 1 = ASK
              id: id,
              auto_withdraw: false,
            },
          }]
        }, {
          blocksBehind: 3,
          expireSeconds: 30,
        });
      } catch (e) {
        let s = e.toString().replace('Error: ','').replace('assertion failure with message: ', '')
        swal({
          icon: "error",
          text: s.charAt(0).toUpperCase() + s.slice(1),
        });
        // alert('\nCaught exception: ' + e);
        // if (e instanceof RpcError)
        //   alert(JSON.stringify(e.json, null, 2));
      }

    },
  }
};
</script>

<style scoped>
.theme--light.v-sheet {
  background-color: inherit;
  color: rgba(0, 0, 0, 0.87);
}
.Header {
  text-transform: uppercase;
  font-size: 2em;
}
.SubHeader {
  font-weight: bold;
}
.v-application .success {
    background-color: #9205ff !important;
    border-color: #9205ff !important;
}

.buttonStyle {
  border-color: rgba(255,0,0,0.8) !important;
  transition: background-color 150ms ease-in-out;
}

.buttonStyle:hover {
  background-color: rgba(255, 0, 0, 0.25) !important;
  /* color: var(--v-background-base) !important; */
  /* font-weight: bolder !important; */
}
</style>