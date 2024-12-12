<template>

  <v-container>

    <v-row>
      <v-col cols="12">
          <v-img
            :src="require('../assets/OCI.png')"
            class="my-6"
            contain
            height="150"
          ></v-img>
      </v-col>
    </v-row>

    <v-row>
      <v-col
        cols="12"
        align="center"
        justify="center"
      >
        <h3 class="demo-text" style="letter-spacing: 0.1em;font-size: 1.7em;">EXCHANGE DEMO</h3>
      </v-col>
    </v-row>

    <v-row>
      <v-col
        cols="6"
        offset="3"
        align="center"
        justify="center"
      >

        <v-row>
          <v-col cols="12" align="center" justify="center">
            <v-card outlined tile class="px-6 py-3">
              <v-row>
                <v-col
                  cols="12"
                  align="center"
                  justify="center"
                >
                  <v-radio-group v-model="endpoint">
                    <v-radio
                      v-for="n in endpointList"
                      :key="n"
                      :label="n"
                      :value="n"
                    >
                    </v-radio>
                  </v-radio-group>
                </v-col>

                <v-col
                  cols="12"
                  v-if="endpoint === 'Custom'"
                  style="padding-top: 0 !important;"
                  class="mt-1"
                >
                  <input
                    class="myInput"
                    type="text"
                    placeholder="BLOCKCHAIN ENDPOINT"
                    v-model="customEndpoint"
                  />
                </v-col>
              </v-row>
            </v-card>
          </v-col>
        </v-row>

        <v-row class="mt-3" v-if="dialogLoading">
          <v-col
            cols="12"
            align="center"
            justify="center"
          >
            <v-progress-circular
              indeterminate
              color="ociaccent"
            ></v-progress-circular>
          </v-col>
        </v-row>

        <v-row class="mt-3">
          <v-col cols="12" align="center" justify="center">
            <v-card outlined tile class="px-6">
              <v-row>
                <v-col
                  cols="12"
                  align="start"
                  style="padding-top: 1.75em !important;padding-bottom: 1.75em !important;"
                >
                  <input
                    class="myInput"
                    type="text"
                    placeholder="ACCOUNT NAME"
                    pattern="[a-z0-5]"
                    maxlength="12"
                    v-model="account"
                    v-on:keypress="keyValidation($event)"
                  />
                </v-col>

                <v-col
                  cols="12"
                  align="start"
                  style="padding-bottom: 1.0em !important;padding-top: 0 !important;"
                >
                  <input class="myInput" type="password" placeholder="PRIVATE KEY" v-model="privateKey"/>
                </v-col>

                <v-col
                  cols="12"
                  align="center"
                  justify="center"
                  style="padding-bottom: 0 !important;padding-top: 0 !important;"
                >
                  <v-btn
                    class="createAccount"
                    color="accent"
                    text large
                    outlined
                    depressed
                    @click="setChain"
                  >
                    LOGIN
                  </v-btn>
                </v-col>

                <v-col
                  cols="12"
                  align="center"
                  justify="center"
                  style="padding-bottom: 1em !important;padding-top: 0 !important;"
                >
                  <v-row justify="center">
                    <v-dialog v-model="dialog" persistent max-width="600px">
                      <template v-slot:activator="{ on }">
                        <v-btn
                          class="createAccount"
                          color="accent"
                          text
                          large
                          outlined
                          depressed
                          dark
                          v-on="on"
                          @click="openCreateAccountForm"
                        >
                          CREATE ACCOUNT
                        </v-btn>
                      </template>
                      <v-card>
                        <v-card-title>
                          <span class="headline" style="color: var(--v-primay-base)">Create Account</span>
                        </v-card-title>
                        <v-card-text style="padding-bottom: 0;">
                          <v-container>
                            <v-row v-if="!loading">
                              <v-col cols="12" style="padding-bottom: 1em !important;padding-top: 1.75em !important;">
                                <input
                                  class="myAccountInput"
                                  placeholder="ACCOUNT NAME :  1-12 characters using a-z or 1-5 only"
                                  required
                                  type="text"
                                  pattern="[a-z0-5]"
                                  maxlength="12"
                                  v-model="newAccountName"
                                  v-on:keypress="keyValidation($event)"
                                />
                              </v-col>
                              <!-- Turning Public Key off because it is irrelevant to users -->
                              <!-- <v-col cols="12" style="padding-bottom: 0 !important;padding-top: 0 !important;">
                                <v-row>
                                  <v-col cols="2" align="start" justify="center" class="keyColumn" style="padding-right: 0;">
                                    <div/>
                                    <h4 style="color: var(--v-secondary-base);font-weight: lighter;">PUBLIC KEY:</h4>
                                  </v-col>
                                  <v-col cols="9" align="end" justify="center" class="keyColumn">
                                    <div/>
                                    <h4 style="color: var(--v-secondary-base);font-weight: lighter;" >{{ newPublicKey}}</h4>
                                  </v-col>
                                  <v-col cols="1" align="end" justify="center" class="keyColumn">
                                    <div/>
                                  </v-col>
                                </v-row>
                              </v-col> -->
                              <v-col cols="12" style="padding-bottom: 2em !important;padding-top: 0 !important;">
                                <v-row>
                                  <v-col cols="2" align="start" justify="center" class="keyColumn" style="padding-right: 0;">
                                    <h4 style="color: var(--v-secondary-base);font-weight: lighter;">PRIVATE KEY:</h4>
                                  </v-col>
                                  <v-col cols="9" align="end" justify="center" class="keyColumn">
                                    <h4 style="color: var(--v-ociaccent-base);">{{ newPrivateKey}}</h4>
                                  </v-col>
                                  <v-col cols="1" align="start" justify="center" class="keyColumn">
                                    <!-- <v-tooltip top open-on-click="true" color="background"> -->
                                    <v-tooltip top color="background">
                                      <template v-slot:activator="{ on }">
                                        <!-- Icons made by <a href="https://www.flaticon.com/authors/freepik" title="Freepik">Freepik</a> from <a href="https://www.flaticon.com/" title="Flaticon"> www.flaticon.com</a> -->
                                        <v-img
                                          type="button"
                                          :src="require('../assets/copy-icon.png')"
                                          contain
                                          height="20"
                                          v-clipboard:copy="newPrivateKey"
                                          v-clipboard:success="onCopy"
                                          v-clipboard:error="onError"
                                          v-on="on"
                                        ></v-img>
                                      </template>
                                      <span style="color: var(--v-ociaccent-base);">{{ tooltipMessage }}</span>
                                    </v-tooltip>
                                  </v-col>
                                </v-row>
                              </v-col>
                            </v-row>
                            <div style="color: var(--v-secondary-base);text-align: right;" v-if="!loading">
                              <small>Copy and paste your PRIVATE KEY somewhere safe.  You will need it to login!</small>
                            </div>
                            <v-row>
                              <v-col cols="12" align="center" justify="center" v-if="loading">
                                <p style="color: var(--v-primay-base)">Creating account...</p>
                                <v-progress-circular
                                  indeterminate
                                  color="ociaccent"
                                ></v-progress-circular>
                              </v-col>
                            </v-row>
                          </v-container>
                        </v-card-text>
                        <v-card-actions>
                          <v-spacer></v-spacer>
                          <v-btn color="blue" text @click="dialog = false">CLOSE</v-btn>
                          <v-btn class="dialogButton" color="blue" text @click="createAccount">CREATE</v-btn>
                        </v-card-actions>
                      </v-card>
                    </v-dialog>
                  </v-row>
                </v-col>
              </v-row>
            </v-card>
          </v-col>
        </v-row>

      </v-col>
    </v-row>

  </v-container>

</template>

<script>
import swal from 'sweetalert';
const axios = require('axios');
let {Keygen} = require('eosjs-keygen')

export default {
  name: 'Landing',

  components: {
  },
  data: () => ({
    endpoint: 'OCI Testnet',
    customEndpoint: null,
    endpointList: [
      'OCI Testnet',
      'Custom'
    ],
    account: null,
    privateKey: null,
    dialog: false,
    newAccountName: null,
    newPublicKey: null,
    newPrivateKey: null,
    tooltipMessage: 'Copy Private Key',
    dialogLoading: false,
    loading: false
  }),
  mounted () {
    // this.setAccount('trader1alice')
    // this.privateKey = '5JLL42ShsECocztJmQReVmWPk9MmgXMkun9DgQQokY6nk5mrXJZ'
  },
  methods: {
    onCopy: function () {
      this.tooltipMessage = 'Private Key Copied!'
    },
    onError: function (e) {
      this.tooltipMessage = 'Failed to copy texts. ' + e.text
    },
    setAccount(account) {
      this.account = account
      this.$store.dispatch('setAccount', this.account)
    },
    setChain() {
      if(this.endpoint === 'OCI Testnet') {
        this.customEndpoint = 'http://exchange-69655700.us-east-2.elb.amazonaws.com:8888'
      }
      this.newPublicKey = null
      this.newPrivateKey = null
      this.$store.dispatch('setAccount', this.account)
      this.$store.dispatch('setEndpoint', { endpoint: this.customEndpoint })
      this.$store.dispatch('setPrivateKey', this.privateKey)

      this.$store.dispatch('setListingPageActive', false)
      this.$store.dispatch('setAccountPageActive', false)
      this.$store.dispatch('setMarketPageActive', true)
      this.$router.push('/dashboard')
    },
    keyValidation(evt) {
      evt = (evt) ? evt : window.event;
      var charCode = (evt.which) ? evt.which : evt.keyCode;
      if ((charCode > 48 && charCode < 54 || charCode > 96 && charCode < 123) || charCode == 46) {
        return true;
      } else {
        evt.preventDefault();
      }
    },
    openCreateAccountForm() {
      this.dialogLoading = true
      Keygen.generateMasterKeys().then(keys => {
        // only use single owner key for simplicity
        // alert(JSON.stringify(keys))
        this.newPublicKey = keys.publicKeys.owner
        this.newPrivateKey = keys.privateKeys.owner

        this.dialogLoading = false
        this.tooltipMessage = 'Copy Private Key'
      })
    },
    createAccount() {
      this.loading = true

      axios.post('https://tnfn3b5e49.execute-api.us-east-2.amazonaws.com/dev/account', {
        accountName: this.newAccountName,
        publicKey: this.newPublicKey,
      })
      .then( (response) => {
        let message = null

        if (JSON.stringify(response.data.statusCode) == 200)
          message = JSON.stringify(response.data.body.message) // account successfully created

        swal({
          text: message.replace(/"/g, ""),
        })
      })
      .catch( (error) => {
        swal({
          icon: "error",
          text: error,
        })
        // console.log(error);
      })
      .finally( () => {
        this.loading = false
      })
    }
  }
};
</script>

<style scoped>
h1 {
  color: var(--v-secondary-base);
}

.keyColumn {
  color: var(--v-secondary-base);
  font-weight: lighter;
  padding-top: 0.5%;
  padding-bottom: 0.5%;
}

input.myInput {
  border-bottom: 0.5px solid var(--v-accent-base) !important;
  /* padding: 0px 0px 0px 5px !important; */
  width: 100% !important;
}

input.myInput:focus {
  outline: none;
}

input.myAccountInput {
  border-bottom: 0.5px solid var(--v-accent-base) !important;
  font-size: 1.1em;
  width: 100% !important;
}

input.myAccountInput:focus {
  outline: none;
}

::placeholder {
  color: #d8d8d8;
  opacity: 0.5; /* Firefox */
}

.v-input--selection-controls {
    margin-top: 0 !important;
    padding-top: 0 !important;
}

.depositButtonStyle {
  border-color: var(--v-accent-base) !important;
  transition: background-color 150ms ease-in-out;
  font-weight: bold;
  width: 100%;
}

.depositButtonStyle:hover {
  /* background-color: rgba(35, 44, 54, 0.25) !important; */
  background-color: var(--v-accent-base) !important;
  color: var(--v-background-base) !important;
}

.createAccount {
  border: none;
  transition: background-color 150ms ease-in-out;
  font-weight: bold;
  width: 100%;
}

.createAccount:hover {
  border-color: var(--v-accent-base) !important;
  background-color: var(--v-accent-base) !important;
  color: var(--v-background-base) !important;
}

/* turn off that peasky auto shading when hovering over */
.v-btn:before {
  display: none;
}

.dialogButton {
  transition: background-color 150ms ease-in-out;
  transition: font-size 150ms ease-in-out;
  /* transition: font-size 150ms ease-out; */
  background-color: transparent;
  /* font-size: 1em; */
  font-weight: bolder;
}

.dialogButton:hover {
  background-color: transparent;
  font-size: 1em;
  font-weight: bolder;
}

.dialogButton:focus {
  background-color: transparent;
  font-size: 1em;
  font-weight: bolder;
}

</style>