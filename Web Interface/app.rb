require 'rubygems'
require 'sinatra'
require 'mqtt'
require 'sinatra/reloader' if development?

set :bind, '0.0.0.0'

serverTopic = '/system_name/server'
serverAck = '/system_name/server_ack'

client = MQTT::Client.new(:host => '127.0.0.1', :username => 'mosquitto', :password => 'password',  :keep_alive => 120)
client.connect

client.subscribe(serverTopic)
#client.publish(serverAck, 'Hello from the server side', false, 1)  


#client.get do |serverTpoic, message|
#	puts message
#end

get '/' do
  # load these from the db
  
  client.publish(serverAck, 'Some text', false, 1)

  serverTopic, message = client.get
  
  puts message.split(",")
  message.each do |count|  
    @sensors = [
      {
        name: message[count],
        temp: message[count + 1],
        hum: message[count+2]
      },
    ]
    count += 2
  end
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
