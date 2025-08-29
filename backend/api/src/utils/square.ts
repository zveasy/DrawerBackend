import { Client, Environment } from 'square';
import { config } from './config.js';

export function getSquareClient() {
  const environment = config.SQUARE_ENV === 'production' ? Environment.Production : Environment.Sandbox;
  const client = new Client({
    accessToken: config.SQUARE_ACCESS_TOKEN,
    environment,
  });
  return client;
}
