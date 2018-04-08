require 'rubygems'
require 'sinatra'
require 'mqtt'
require 'sinatra/reloader' if development?

#client = MQTT::Client.new
#client.host = '127.0.0.1'
#client.ssl = true
#client.username = 'mosquitto'
#client.password = 'password'
#client.connect(host)

MQTT::Client.connect('127.0.0.1') do |client|
	client.publish('hello', 'message')
end

get '/' do
    # load these from the db
  @sensors = [
    {
      name: 'Living room',
      temp: '24.5 °C',
      hum: '41.5 %'
    },
    {
      name: 'Bathroom',
      temp: '24 °C',
      hum: '50 %' 
    },
    {
      name: 'Kitchen',
      temp: '22 °C',
      hum: '40.5 %'
    },
    {
      name: 'Sensor 004',
      temp: '22 °C',
      hum: '40.5 %'
    },
  ]
  erb :index
end

get '/statistics' do
  erb :statistics
end

get '/config' do
  # load these from the db
  @sensors = [
    {
      name: 'Living room',
      enabled: true,
      op_mode: 'sensor'
    },
    {
      name: 'Sensonr name2',
      enabled: false,
      op_mode: 'IDLE' 
    },
    {
      name: 'Sensor name3',
      enabled: true,
      op_mode: 'relay'
    },
  ]
  erb :config
end
