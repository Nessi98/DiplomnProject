require 'rubygems'
require 'sinatra'
require 'mqtt'
require 'sqlite3'
require 'sinatra/reloader' if development?

set :bind, '0.0.0.0'

serverTopic = '/system_name/server'
serverAction = '/system_name/server_action'

client = MQTT::Client.new(:host => '127.0.0.1', :username => 'mosquitto', :password => 'password',  :keep_alive => 120)
client.connect

client.subscribe(serverTopic)
#client.publish(serverAck, 'Hello from the server side', false, 1)  

db = SQLite3::Database.new "../RaspiClient/automation.db"
db.results_as_hash = true

get '/' do
	@sensors = []
	db.execute( "SELECT * FROM SensorUnit as su LEFT JOIN Data as d ON su.id = d.unitID ORDER BY d.time ASC LIMIT 1" ) do |sensor|
		@sensors << sensor	
	end
	erb :index
end

# Sends message to the C client to get data about the senosr unit in the DB
get '/statistics' do
	@sensors = []

	db.execute( "SELECT * FROM SensorUnit" ) do |sensor|
		@sensors << sensor
	end

	@id = @sensors.first['id']
	@period = 'daily'

	if params[:id] && !params[:id].empty?
		@id = params[:id]
	end
	if params[:period] && !params[:id].empty?
		@period = params[:period]
	end

	now = Time.now
	query = case @period
		when 'monthly'
			"SELECT AVG(temp) as temp, AVG(hum) as hum, strftime('%d', time) as time FROM Data WHERE unitID = #{@id} AND date(time) BETWEEN '#{now.year}-01-#{now.month.to_s.rjust(2, '0')}' AND '#{now.year}-#{now.day.to_s.rjust(2, '0')}-#{now.month.to_s.rjust(2, '0')}' GROUP BY strftime('%Y-%m-%d', time)"
		 when 'yearly'
			"SELECT AVG(temp) as temp, AVG(hum) as hum, strftime('%m', time) as time FROM Data WHERE unitID = #{@id} AND date(time) BETWEEN '#{now.year}-01-01' AND '#{now.year}-#{now.day.to_s.rjust(2, '0')}-#{now.month.to_s.rjust(2, '0')}' GROUP BY strftime('%m', time)"
		 else 'daily'
			 "SELECT AVG(temp) as temp, AVG(hum) as hum, strftime('%H', time) as time FROM Data WHERE unitID = #{@id} AND date(time) = '#{now.year}-#{now.day.to_s.rjust(2, '0')}-#{now.month.to_s.rjust(2, '0')}' GROUP BY strftime('%H', time)"
		 end
	data = { 'dataPoints' => [] }
	db.execute(query) do |row|
		data['dataPoints'] << row			
	end

	@sensorData = [ data ]

	erb :statistics
end

get '/config' do
	@sensors = []
	db.execute( "SELECT * FROM SensorUnit" ) do |sensor|
		sensor['enabled'] = sensor['opMode'] != 'IDLE'
		@sensors << sensor	
	end
	p @sensors
	erb :config
end


get '/sensor' do
	puts "Change mode: mode = #{params[:mode]} unitID = #{params[:id]}"
	
	client.publish(serverAction, "Change mode;opMode=#{params[:mode]};unitID=#{params[:id]}", false, 1)
	
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
