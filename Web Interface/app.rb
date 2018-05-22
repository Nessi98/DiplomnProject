require 'rubygems'
require 'sinatra'
require 'mqtt'
require 'sinatra/reloader' if development?

set :bind, '0.0.0.0'

serverTopic = '/system_name/server'
serverAction = '/system_name/server_action'

client = MQTT::Client.new(:host => '127.0.0.1', :username => 'mosquitto', :password => 'password',  :keep_alive => 120)
client.connect

client.subscribe(serverTopic)
#client.publish(serverAck, 'Hello from the server side', false, 1)  

get '/' do
  # load these from the db
  
  client.publish(serverAction, 'Real Time', false, 1)

  serverTopic, message = client.get
  data = message.split(",")
 
  count = 0 
  sensorsCount = 0

  nameArr  = []
  tempArr = []
  humArr = []

  data.each do |d|
  
    case  
    when count == 0
	nameArr[sensorsCount] = d
	count += 1
    when count == 1
	tempArr[sensorsCount] = d
	count += 1
    else
	humArr[sensorsCount] = d
	count = 0
	sensorsCount += 1
    end
  end
  count = 0

  @sensors = Array.new
  while count < sensorsCount do
    @sensors << { :name => nameArr[count], :temp => tempArr[count], :hum => humArr[count]}
    count += 1
  end

  erb :index
end

get '/statistics' do
  client.publish(serverAction, "Statistics", false, 1)
  erb :statistics
end

get '/config' do
  # load these from the db
  client.publish(serverAction, 'Config', false, 1)

  serverTopic, message = client.get
 
  counter = 0
  sensorCount = 0

  nameArr = []
  opModeArr = []
  enabledArr = []

  data = message.split(",")
  
  data.each do |d|

    case counter
    when 0
      nameArr[sensorCount] = d
      counter += 1
    else
      if d.include? "IDLE"
	enabledArr[sensorCount] = false
      else
	enabledArr[sensorCount] = true
      end
      opModeArr[sensorCount] = d
      sensorCount += 1
      counter = 0
    end
  end

  counter = 0

  @sensors = Array.new
  while counter < sensorCount do
    @sensors << { :name => nameArr[counter], :enabled => enabledArr[counter], :op_Mode => opModeArr[counter]}
    counter += 1
  end

   erb :config
end
