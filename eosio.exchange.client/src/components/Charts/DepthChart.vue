<template>
  <div id="depthchart"></div>
</template>


<script>
import * as am4core from "@amcharts/amcharts4/core";
import * as am4charts from "@amcharts/amcharts4/charts";
import am4themes_themes from "@amcharts/amcharts4/themes/dark.js";

am4core.useTheme(am4themes_themes);

export default {
  name: 'DepthChart',
  props: ['market', 'base', 'quote', 'bids', 'asks'],
  data: () => ({
    chart: null,
    interval: null,
  }),
  computed: {

  },
  methods: {
    drawChart () {
      /* TEST CHART */
      let bidData = []
      let askData = []

      for(let i = 0; i < this.bids.length; i++) {
        bidData.push([
          this.bids[i].price.quantity.replace(/\s[A-Z]+/g, ''), // price
          this.bids[i].volume,  // volume
        ])
      }

      for(let i = 0; i < this.asks.length; i++) {
        askData.push([
          this.asks[i].price.quantity.replace(/\s[A-Z]+/g, ''), // price
          this.asks[i].volume,  // volume
        ])
      }

      // Add Data
      // chart.data = bidData

      // Add category axis
      // let categoryAxis = chart.xAxes.push(new am4charts.CategoryAxis());
      // categoryAxis.dataFields.category = "price";
      // Add value axis
      // chart.yAxes.push(new am4charts.ValueAxis());
      // Add series
      // let series = chart.series.push(new am4charts.ColumnSeries());
      // series.name = "Web Traffic";
      // series.dataFields.categoryX = "price";
      // series.dataFields.valueY = "volume";
      /* TEST CHART */

      // Function to process (sort and calculate cummulative volume)
      function processData(list, type, desc) {

        // Convert to data points
        for(var i = 0; i < list.length; i++) {
          list[i] = {
            value: Number(list[i][0]),
            volume: Number(list[i][1]),
          }
        }

        // alert('list')
        // alert(list[0])

        // Sort list just in case
        list.sort(function(a, b) {
          if (a.value > b.value) {
            return 1;
          }
          else if (a.value < b.value) {
            return -1;
          }
          else {
            return 0;
          }
        });

        // Calculate cummulative volume
        if (desc) {
          for(var j = list.length - 1; j >= 0; j--) {
            if (j < (list.length - 1)) {
              list[j].totalvolume = list[j+1].totalvolume + list[j].volume;
            }
            else {
              list[j].totalvolume = list[j].volume;
            }
            let dp = {};
            dp["value"] = list[j].value;
            dp[type + "volume"] = list[j].volume;
            dp[type + "totalvolume"] = list[j].totalvolume;
            res.unshift(dp);
          }
        }
        else {
          for(var k = 0; k < list.length; k++) {
            if (k > 0) {
              list[k].totalvolume = list[k-1].totalvolume + list[k].volume;
            }
            else {
              list[k].totalvolume = list[k].volume;
            }
            let dp = {};
            dp["value"] = list[k].value;
            dp[type + "volume"] = list[k].volume;
            dp[type + "totalvolume"] = list[k].totalvolume;
            res.push(dp);
          }
        }

      }

      // Init
      let res = [];
      processData(bidData, "asks", false);
      processData(askData, "bids", true);

      this.chart.data = res;
      this.chart.invalidateRawData()
    },
    refresh () {
      var self = this
      this.interval = setInterval(function () {
        self.drawChart()
      }, 1000);
    },
  },
  mounted() {
    let base = this.market.value.quantity.replace(/\d+./g, '')
    // let base = this.base  // only loads the second time for some reason
    let quote = this.quote.symbol
    this.refresh()
    // this.drawChart()

    // Create chart
    this.chart = am4core.create("depthchart", am4charts.XYChart);
    var chart = this.chart

    // Set up precision for numbers
    chart.numberFormatter.numberFormat = "#,###.####";

    // Create axes
    let xAxis = chart.xAxes.push(new am4charts.CategoryAxis());
    xAxis.dataFields.category = "value";
    // xAxis.dataFields.category = "price";
    xAxis.renderer.grid.template.location = 0;
    xAxis.renderer.minGridDistance = 50;
    xAxis.title.text = "Price (" + base + "/" + quote + ")";
    xAxis.renderer.grid.disabled = true;
    xAxis.renderer.grid.template.disabled = true;

    let yAxis = chart.yAxes.push(new am4charts.ValueAxis());
    yAxis.title.text = "Volume";
    // yAxis.renderer.grid.disabled = true;
    // yAxis.renderer.grid.template.disabled = true;

    // Create series
    let series = chart.series.push(new am4charts.StepLineSeries());
    series.dataFields.categoryX = "value";
    // series.dataFields.categoryX = "price";
    series.dataFields.valueY = "bidstotalvolume";
    // series.dataFields.valueY = "volume";
    series.strokeWidth = 1;
    series.stroke = am4core.color("#0f0");
    series.fill = series.stroke;
    series.fillOpacity = 0.25;
    series.tooltipText = "Ask: [bold]{categoryX}[/]\nTotal volume: [bold]{valueY}[/]\nVolume: [bold]{bidsvolume}[/]"

    let series2 = chart.series.push(new am4charts.StepLineSeries());
    series2.dataFields.categoryX = "value";
    series2.dataFields.valueY = "askstotalvolume";
    series2.strokeWidth = 1;
    series2.stroke = am4core.color("#f00");
    series2.fill = series2.stroke;
    series2.fillOpacity = 0.25;
    series2.tooltipText = "Ask: [bold]{categoryX}[/]\nTotal volume: [bold]{valueY}[/]\nVolume: [bold]{asksvolume}[/]"

    let series3 = chart.series.push(new am4charts.ColumnSeries());
    series3.dataFields.categoryX = "value";
    // series3.dataFields.categoryX = "price";
    series3.dataFields.valueY = "bidsvolume";
    // series3.dataFields.valueY = "volume";
    series3.strokeWidth = 0;
    series3.fill = am4core.color("#000");
    series3.fillOpacity = 0; // 0.2;

    let series4 = chart.series.push(new am4charts.ColumnSeries());
    series4.dataFields.categoryX = "value";
    series4.dataFields.valueY = "asksvolume";
    series4.strokeWidth = 0;
    series4.fill = am4core.color("#000");
    series4.fillOpacity = 0; // 0.2

    // Add cursor
    chart.cursor = new am4charts.XYCursor();
  },
  beforeDestroy() {
    clearInterval(this.interval)
    if (this.chart) {
      this.chart.dispose();
    }
  },
};
</script>

<style>
#depthchart {
  width: 100%;
  height: 370px;
}
</style>