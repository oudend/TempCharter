const validator = require("express-json-validator-middleware");
const express = require("express");
const cors = require("cors");
const ip = require("ip");
const app = express();
const os = require("os");

ipv4 = os.networkInterfaces()["Wi-Fi"].at(-1).address;

app.use(express.json());

app.use(
  cors({
    origin: "http://127.0.0.1:3000", //? make cors work for the website(only works with that exact address)
  })
);

app.set("port", process.env.PORT || 3000);

/**
 * Define a JSON schema.
 */
const addressSchema = {
  type: "object",
  required: ["type", "data"],
  properties: {
    type: {
      type: "string",
      enum: ["temperature", "humidity", "heatIndex"],
    },
    data: {
      type: "array",
    },
  },
};

const { validate } = new validator.Validator();

var data = { type: "", data: [] };

app.post("/send", validate({ body: addressSchema }), (req, res) => {
  console.log(req.body);
  data = req.body;

  res.sendStatus(200);
});

app.get("/get", (req, res) => {
  res.status(200).json(data);
});

/**
 * Error handler middleware for validation errors.
 */
app.use((error, request, response, next) => {
  // Check the error is a validation error
  console.log(error instanceof validator.ValidationError);
  if (error instanceof validator.ValidationError) {
    // Handle the error
    console.log(request);
    response.status(400).send(error.validationErrors);
    next();
  } else {
    // Pass error on if not a validation error
    next(error);
  }
});

app.listen(app.get("port"), ipv4, () => {
  console.log("Server running on port 3000", ipv4);
});
