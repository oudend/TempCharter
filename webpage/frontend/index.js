const ctx = document.getElementById("myChart");

var labelCount = 0;

var form = document.getElementById("ipForm");
const ipInput = document.getElementById("ipInput");
function handleForm(event) {
  event.preventDefault();
}
form.addEventListener("submit", handleForm);

window.onload = function () {
  let ip = localStorage.getItem("ip") ?? "";
  ipInput.setAttribute("value", ip);

  if (ip != "") updateData(ip);

  let vh = window.innerHeight * 0.01;
  document.documentElement.style.setProperty("--vh", `${vh}px`);
};

window.onresize = (e) => {
  let vh = window.innerHeight * 0.01;
  // Then we set the value in the --vh custom property to the root of the document
  document.documentElement.style.setProperty("--vh", `${vh}px`);
};

function ValidateIPaddress(inputText) {
  var ipformat =
    /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
  if (inputText.value.match(ipformat)) {
    ip = inputText.value;
    updateData(ip);
    localStorage.setItem("ip", ip);
    return true;
  }
  return false;
}

let docStyle = getComputedStyle(document.documentElement);

// Note: changes to the plugin code is not reflected to the chart, because the plugin is loaded at chart construction time and editor changes only trigger an chart.update().
const plugin = {
  id: "customCanvasBackgroundColor",
  beforeDraw: (chart, args, options) => {
    const { ctx } = chart;
    ctx.save();
    ctx.globalCompositeOperation = "destination-over";
    ctx.fillStyle = options.color || "#99ffff";
    ctx.fillRect(0, 0, chart.width, chart.height);
    ctx.restore();
  },
};

Chart.defaults.borderColor = docStyle.getPropertyValue("--accent");
// Chart.defaults.labelColor = docStyle.getPropertyValue("--accent");

const labels = [];
const data = {
  labels: labels,
  datasets: [
    {
      label: "Temperature",
      data: [],
      fill: true,
      // borderColor: docStyle.getPropertyValue("--text"),
      tension: 0.1,
    },
  ],
};

Chart.defaults.color = docStyle.getPropertyValue("--text");

var chart = new Chart(ctx, {
  type: "line",
  data: data,
  options: {
    responsive: true,
    maintainAspectRatio: false,
    height: 100,
    plugins: {
      customCanvasBackgroundColor: {
        color: docStyle.getPropertyValue("--secondary"),
      },
    },
  },
  plugins: [plugin],
});

function addPoint(value) {
  chart.data.datasets[0].data.push(value);
  chart.data.labels.push(labelCount); //.push(```${7}```);
  chart.update();
  labelCount++;
}

function updateData(ip) {
  if (ip != null) {
    let url = `http://${ip}:3000/get`;
    fetch(url)
      .then((res) => res.json())
      .then((data) => {
        chart.data.datasets[0].data = [];
        chart.data.labels = [];
        labelCount = 0;
        chart.data.datasets[0].label = data.type;
        chart.update();

        for (var i = 0; i < data.data.length; i++) {
          addPoint(data.data[i]);
        }
      })
      .catch((err) => {
        throw err;
      });
  }
}
