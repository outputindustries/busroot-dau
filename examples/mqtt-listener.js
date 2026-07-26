#!/usr/bin/env node

/**
 * Busroot DAU MQTT Listener Example
 * 
 * This simple Node.js script demonstrates how to subscribe to and process
 * data published by Busroot DAU devices.
 * 
 * Install dependencies:
 *   npm install mqtt
 * 
 * Usage:
 *   node mqtt-listener.js [broker-url]
 * 
 * Example:
 *   node mqtt-listener.js mqtt://localhost:1883
 */

const mqtt = require('mqtt');

// Configuration
const BROKER_URL = process.argv[2] || 'mqtt://localhost:1883';
const TOPIC_PATTERN = '+/busroot/v2/dau/+';  // Subscribes to all busroot DAU devices

console.log('Busroot DAU MQTT Listener');
console.log('========================');
console.log(`Connecting to: ${BROKER_URL}`);
console.log(`Topic pattern: ${TOPIC_PATTERN}`);
console.log('');

// Connect to MQTT broker
const client = mqtt.connect(BROKER_URL);

client.on('connect', () => {
  console.log('✓ Connected to MQTT broker');
  
  // Subscribe to busroot DAU topics
  client.subscribe(TOPIC_PATTERN, (err) => {
    if (err) {
      console.error('✗ Subscription failed:', err.message);
      process.exit(1);
    }
    console.log('✓ Subscribed to topic pattern:', TOPIC_PATTERN);
    console.log('');
    console.log('Waiting for messages...');
    console.log('');
  });
});

client.on('message', (topic, message) => {
  try {
    // Parse the JSON message
    const data = JSON.parse(message.toString());
    
    // Extract device ID from topic (format: prefix/busroot/v2/dau/deviceId)
    const topicParts = topic.split('/');
    const deviceId = topicParts[topicParts.length - 1];
    
    // Display message
    console.log(`[${new Date().toISOString()}] Device: ${deviceId}`);
    console.log('  Data:', JSON.stringify(data, null, 2));
    
    // Example: Extract key metrics
    if (data.ver) console.log(`  Version: ${data.ver}`);
    if (data.rssi !== undefined) console.log(`  WiFi Signal: ${data.rssi} dBm`);
    
    // Pulse counters
    const counters = ['c1', 'c2', 'c3', 'c4', 'c5', 'c6'];
    const activeCounts = counters
      .filter(c => data[c] !== undefined && data[c] > 0)
      .map(c => `${c}=${data[c]}`);
    if (activeCounts.length > 0) {
      console.log(`  Counters: ${activeCounts.join(', ')}`);
    }
    
    // Analog inputs
    if (data.a7 !== undefined || data.a8 !== undefined) {
      console.log(`  Analog: a7=${data.a7 || 0}, a8=${data.a8 || 0}`);
    }
    
    // Modbus energy meter data (if present)
    if (data.kWh1 !== undefined) {
      console.log(`  Modbus Device 1:`);
      console.log(`    Energy: ${data.kWh1} kWh`);
      if (data.p1v1) console.log(`    Voltages: ${data.p1v1}V, ${data.p2v1}V, ${data.p3v1}V`);
      if (data.p1a1) console.log(`    Currents: ${data.p1a1}A, ${data.p2a1}A, ${data.p3a1}A`);
      if (data.pf1) console.log(`    Power Factor: ${data.pf1}`);
    }
    
    console.log('');
    
  } catch (error) {
    console.error('Error parsing message:', error.message);
    console.error('Raw message:', message.toString());
    console.log('');
  }
});

client.on('error', (error) => {
  console.error('✗ MQTT Error:', error.message);
});

client.on('close', () => {
  console.log('Connection closed');
});

// Handle graceful shutdown
process.on('SIGINT', () => {
  console.log('\nShutting down...');
  client.end();
  process.exit(0);
});
