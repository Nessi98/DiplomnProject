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

get '/' do
  # load these from the db
  
  client.publish(serverAck, 'Some text', false, 1)

  serverTopic, message = client.get
  data = message.split(",")
 
  count = 0 
  i = 0

  names = []
  temps = []
  hums = []
  data.each do |d|
  
    case  
    when count == 0
	names[i] = d
	puts "Array of names", names
	count += 1
    when count == 1
	temps[i] = d
	puts temps
	count += 1
    else
	hums[i] = d
	puts hums
	count = 0
	i += 1
    end
  end
  count = 0

  @sensors = Array.new
  while count < i do
    puts names[count]
    @sensors << { :name => names[count], :temp => temps[count], :hum => hums[count]}
    count += 1
  end

  puts @sensors[0]

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
