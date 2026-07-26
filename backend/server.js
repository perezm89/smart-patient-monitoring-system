require('dotenv').config();

const app = require('./src/app');
const connectDatabase = require('./src/config/db');
// Use the port defined in the environment, or default to 5000 during local development.
const PORT = process.env.PORT || 5000;

/*
Starts the backend application.
 Startup order:
 1. Connect to MongoDB.
 2. Start the Express server.
 
 The server only begins accepting requests after a successful
 database connection.
 */
const startServer = async () => {
  await connectDatabase();

  app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
  });
};

startServer();