import Vue from 'vue';
import Vuetify from 'vuetify/lib';

import colors from 'vuetify/lib/util/colors'

Vue.use(Vuetify);

export default new Vuetify({
  theme: {
    options: {
      customProperties: true
    },
    dark: true,
    themes: {
      light: {
        primary: colors.blue.darken1, // #E53935
        secondary: colors.blue.lighten4, // #FFCDD2
        accent: colors.indigo.base, // #3F51B5
      },
      dark: {
        background: "#003E5C",
        overlay: "#004b6e", // rgb(0, 75, 110)
        primary: "F8B475",  // yellow
        secondary: "#FFFFFF", // white
        accent: "#4A90E2",    // blue
        ociaccent: "#80d5de", // light-blue
        good: "#00FF00", // green
        bad: "#FF5B3F", // red
        error: "#FF0000",
      },
    },
  },
});
