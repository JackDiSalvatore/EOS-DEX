<template>

  <div>
    <Nav/>

    <v-container>
      <v-card outlined tile class="flexFit pa-12 ma-12">

        <h1>About</h1><br/>
        <p>
          The OCI EOSIO Token Exchange is a smart contract that allows blockchain users to trade EOSIO
          based Cryptocurrency Tokens without the need of a centralized server.  Users can place limit
          orders at desired prices or perform atomic swaps without having to depsoit Tokens into the
          order book.
        </p>
        <br/>
        <hr/>
        <br/>

        <h2>FAQ</h2><br/>

        <h3>Create Account</h3>
        <p>
          New blockchain accounts can be created and funded with test tokens to evaluate the demo.
          To create an account, on the home page, click "Create Account."  Type an account name.
          <br/>
          Account names must be between 1 and 12 characters, contain a letter a - z, or number 0 - 5.
          They can optionally include a period ".".
          <br/>
          Copy the<code>PRIVATE KEY</code>and save it somewhere safe and secret; this will act as your password.
          Click the<code>CREATE</code>button.  Close this pop-up to log in.
        </p>
        <br/>

        <h3>Log In</h3>
        <p>
          On the home page, type in the account name in the<code>ACCOUNT NAME</code>field and paste your
          private key into the<code>PRIVATE KEY</code>field.  Click the<code>LOGIN</code>button to go to the Markets page.
        </p>
        <br/>

        <h3>Deposit</h3>
        <p>
          After logging in, click on the account name in the top right-hand corner of the screen to be directed to the Accounts page.
          Under the<code>Deposit</code>heading, click on<code>TOKEN</code>to select the Token symbol and enter the number of Tokens you wish to deposit.
          In this version of the demo, the amount deposited must match the exact precision of the currency.
          Click<code>DEPOSIT</code>to deposit the tokens.
        </p>
        <br/>

        <h3>Withdraw</h3>
        <p>
          On the Accounts page, under the<code>Withdraw</code>heading, click on<code>TOKEN</code>to select the Token symbol and enter the number of Tokens
          you wish to withdraw.  In this version of the demo, the amount withdrew must match the exact precision of the currency.
          Click<code>WITHDRAW</code>to withdraw the tokens.
        </p>
        <br/>

        <h3>Trading</h3>
        <p>
          Trading allows users to place orders they wish to be filled.  An order can be to either buy or sell one Token for another.
          To place an order, the user must state the number of Tokens to trade and the price to execute it.  If the order gets filled,
          the Tokens are traded and reflected in both users' exchange balance.
        </p>
        <p>
          On the Markets page, select a market to trade in by clicking a Token from the top navigation bar and then clicking a market pair,
          ie. "EOS / USD".  In the bottom right card, click either<code>BUY</code>or<code>SELL</code>to choose the trading position, then enter
          the<code>Price</code>and<code>Amount</code>. Click the<code>BUY</code>or<code>SELL</code>button to place the trade.
        </p>
        <br/>

        <h3>Atomic Swaps</h3>
        <p>
          Atomic Swaps allow users to quickly "swap" one token for another at a predetermined exchange rate without having to deposit tokens into
          the exchange.  The exchange rate is the current price trading from the market.
        </p>
        <p>
          On the Markets page, click<code>SWAP</code>to the right of a market pair.  Click either<code>BUY</code>or<code>SELL</code> to choose the
          swap position and then enter the<code>Amount</code>.  Click the<code>BUY</code>or<code>SELL</code>button to complete the swap.
        </p>
        <br/>

        <h2>Admin</h2>
        <br/>
        <p>
            Log in as<code>exchange5432</code>to enable admin features.  Admin access is required to use the Listings page.
        </p>
        <br/>

        <h3>Add Token Support</h3>
        <p>
          Click<code>LISTINGS</code>in the navbar, under<code>Supported Tokens</code>, input the information about the new Token, and click<code>ADD</code>.
          All Tokens supported by the exchange are listed below.
        </p>
        <br/>

        <h3>Create Market</h3>
        <p>
          To add a new market, click<code>LISTINGS</code>in the navbar, then under<code>Create Market</code>,
          click the<code>Market</code>button and select the Token to add to the exchange markets.
        </p>
        <br/>

        <h3>Add Trading Pair</h3>
        <p>
          To add a trading pair for a market, click<code>LISTINGS</code>in the navbar, then under<code>Add Trading Pair</code>,
          click<code>MARKET</code>select the market and click<code>TRADING PAIR</code>to select the trading pair Token.
        </p>
        <br/>

      </v-card>
    </v-container>

  </div>

</template>

<script>
import Nav from '../Nav'

export default {
  name: 'AccountInfo',
  props: [],
  components: {
    Nav,
  },
  data: () => ({
  }),
  mounted () {
  },
  computed: {
  },
  methods: {
  },
  beforeDestroy() {
  },
};
</script>

<style scoped>
.flexFit {
  flex: 1;
  height: 100%;
}

h4 {
    color: #94dbe3;
}

hr {
    border: 0;
    height: 1px;
    background: #80d5de;
    background-image: -webkit-linear-gradient(left, #ccc, #80d5de, #ccc);
    background-image: -moz-linear-gradient(left, #ccc, #80d5de, #ccc);
    background-image: -ms-linear-gradient(left, #ccc, #80d5de, #ccc);
    background-image: -o-linear-gradient(left, #ccc, #80d5de, #ccc);
}

code {
  font-family: 'Courier New', Courier, monospace;
  color: var(--v-secondary-base);;
  /* background-color: var(--v-background-base); */
  background-color: transparent;
  box-shadow: none;
  padding: 0;
  margin: 0;
  font-size: 95%;
  /* font-weight: 550; */
}
</style>