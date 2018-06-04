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

# Sends message to the C client to get data about the senosr unit in the DB
get '/statistics' do

	@period = params[:period]
	case @period
	when 'daily'
		puts "Publish for daily stats"
		client.publish(serverAction, "Statistics for the day", false, 1)
	when 'monthly'
		puts "publish for monthly stats"
		client.publish(serverAction, "Statistics for the month", false, 1)
	when 'yearly'
		puts "publish for yearly stats"
		client.publish(serverAction, "Statistics for the year", false, 1)
	else
		redirect '/statistics?period=daily'
	end
	
	serverTopic, message = client.get
	data = message.split(',')
	
	idArr = []
	nameArr = []
	
	count = 0
	sensorCount = 0
	
	data.each do |d|
		case
		when count = 0
			idArr[sensorCount] = d
			count += 1
		else
			nameArr[sensorCount] = d
			
			count = 0
			sensorCount += 1
		end
	end 
	
	count = 0

	@sensors = Array.new
	while count < sensorCount do
		@sensors << { :name => nameArr[count], :temp => tempArr[count], :hum => humArr[count]}
		count += 1
	end
	
	#data.each do |d|
	#	puts d
		#data.split(",").each do |d|
		#end
	#end
	
	@sensorsData = [{
		sensorName: 'Kitchen',
		sensorColor: 'blue',
		
		dataPoints: [{
			temp: 15.7,
			time: 9
		},
		{
			temp: 16.9,
			time: 10
		},
		]
	}]
	
	erb :statistics
end

get '/config' do
	# Send message to the C client to get information for the sensors
	client.publish(serverAction, 'Config', false, 1)

	serverTopic, message = client.get
	 
	counter = 0
	sensorCount = 0

	idArr = []
	nameArr = []
	opModeArr = []
	enabledArr = []

	data = message.split(",")
	  
	data.each do |d|

		case counter
		when 0
			idArr[sensorCount] = d
			counter += 1
		when 1
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
		@sensors << {:id => idArr[counter], :name => nameArr[counter], :enabled => enabledArr[counter], :op_Mode => opModeArr[counter]}
		
		counter += 1
	end

	erb :config
end


get '/sensor' do
	puts "Change mode: mode = #{params[:mode]} unitID = #{params[:id]}"
	
	client.publish(serverAction, "Change mode;mode=#{params[:mode]};unitID=#{params[:id]}", false, 1)
	
	redirect "/config"
end

post '/change_settings' do

	unless params[:name].empty? 
		name = params[:name].gsub("\\", "\\\\").gsub("'", "\\'").gsub("\"", "\\\"")
		puts "Escaped name = #{name}"
		
		client.publish(serverAction, "Change settings;name='#{name}';unitID=#{params[:id]}", false, 1)
	end
	
	unless params[:value].empty?
		puts "Relay value #{params[:value]}"
		
		client.publish(serverAction, "Change settings;relayValue=#{params[:value]};unitID=#{params[:id]}", false, 1)
	end	
	
	redirect "/config"
end

get '/statistics' do
	puts 'Statistics'
end