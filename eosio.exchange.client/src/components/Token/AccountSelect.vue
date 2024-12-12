<template>

  <v-menu offset-y>
    <template v-slot:activator="{ on }">
      <v-btn
        class="depositButtonStyle"
        color="secondary"
        text
        outlined
        depressed
        v-on="on"
      >
        {{ accountName ? accountName : "LOGIN" }}
          <v-img
            :src="require('../assets/down-arrow.svg')"
            class="ml-2"
            height="10"
            width="10"
            contain
          ></v-img>
      </v-btn>
    </template>
    <v-list>
      <v-list-item
        v-for="(account, index) in accounts"
        :key="index"
        @click="() => $emit('selected', account.name)"
      >
        <v-list-item-title >{{ account.name }}</v-list-item-title>
      </v-list-item>
    </v-list>
  </v-menu>

</template>


<script>
export default {
  name: 'AccountSelect',
  props: ['accountName'],

  data: () => ({
    accounts: [
      { name: process.env.VUE_APP_TRADER_ALICE },
      { name: process.env.VUE_APP_TRADER_BARRY },
      { name: process.env.VUE_APP_EXCHANGE_CONTRACT },
    ],
  }),
  methods: {
  }
};
</script>

<style scoped>
.depositButtonStyle {
  border-color: var(--v-accent-base) !important;
  transition: background-color 150ms ease-in-out;
}

.depositButtonStyle:hover {
  background-color: rgba(74, 144, 226, 0.25) !important;
}
</style>