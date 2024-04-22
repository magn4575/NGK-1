#include <iostream>
#include <restinio/all.hpp>
#include <json_dto/pub.hpp>

struct place_t
{
    place_t() = default;

    place_t(
        std::string name,
        double latitude,
        double longitude )
        : m_name{ std::move( name ) }
        , m_lat{ std::move (latitude) }
        , m_lon{ std::move (longitude) }
    {}

    template <typename JSON_IO>
    void json_io( JSON_IO& io )
    {
        io
            & json_dto::mandatory( "name", m_name )
            & json_dto::mandatory( "lat", m_lat )
            & json_dto::mandatory( "lon", m_lon );
    }

    std::string m_name;
    double m_lat;
    double m_lon;
};

struct weather_data_t
{
    weather_data_t() = default;

    weather_data_t(
        std::string ID,
        std::string date,
        std::string time,
        place_t place,
        std::string temperature,
        int humidity )
        : m_ID{ std::move( ID ) }
        , m_date{ std::move( date ) }
        , m_time{ std::move( time ) }
        , m_place{ std::move( place ) }
        , m_temperature{ std::move (temperature) }
        , m_humidity{ std::move (humidity) }
    {}

    template <typename JSON_IO>
    void json_io( JSON_IO& io )
    {
        io
            & json_dto::mandatory( "ID", m_ID )
            & json_dto::mandatory( "date", m_date )
            & json_dto::mandatory( "time", m_time )
            & json_dto::mandatory( "place", m_place )
            & json_dto::mandatory( "temperature", m_temperature )
            & json_dto::mandatory( "humidity", m_humidity );
    }

    std::string m_ID;
    std::string m_date;
    std::string m_time;
    place_t m_place;
    std::string m_temperature;
    int m_humidity;
};




using weather_data_collection_t = std::vector< weather_data_t >;

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

class weather_data_handler_t
{
    public :
	explicit weather_data_handler_t( weather_data_collection_t & weather_data )
		:	m_weather_data( weather_data )
	{}

	weather_data_handler_t( const weather_data_handler_t & ) = delete;
	weather_data_handler_t( weather_data_handler_t && ) = delete;

	auto on_weather_list(
		const restinio::request_handle_t& req, rr::route_params_t ) const
	{
		auto resp = init_resp( req->create_response() );

		resp.set_body(
			"Weather collection (Weather count: " +
				std::to_string( m_weather_data.size() ) + ")\n" );

		for( std::size_t i = 0; i < m_weather_data.size(); ++i )
		{
			//resp.append_body( std::to_string( i + 1 ) + ". " );
			const auto & b = m_weather_data[ i ];
			resp.append_body( "ID " + b.m_ID + "\n" );
            resp.append_body( "Tidspunkt (Dato og klokkeslaet) \n" );
            resp.append_body( "Dato " + b.m_date + "\n" );
	        resp.append_body( "Klokkeslaet " + b.m_time + "\n" );
            resp.append_body( "Sted \n" );
            resp.append_body( "Navn " + b.m_place.m_name + "\n" );
            resp.append_body( "lat " + std::to_string(b.m_place.m_lat) + "\n" );
            resp.append_body( "lon " + std::to_string(b.m_place.m_lon) + "\n" );
            resp.append_body( "Temperatur " + b.m_temperature + "\n" );
            resp.append_body( "Luftfugtighed " + std::to_string(b.m_humidity) + " %\n" );

		return resp.done();
	    }
    }

    private :
	weather_data_collection_t & m_weather_data;

	template < typename RESP >
	static RESP
	init_resp( RESP resp )
	{
		resp
			.append_header( "Server", "RESTinio sample server /v.0.6" )
			.append_header_date_field()
			.append_header( "Content-Type", "text/plain; charset=utf-8" );

		return resp;
	}

	template < typename RESP >
	static void
	mark_as_bad_request( RESP & resp )
	{
		resp.header().status_line( restinio::status_bad_request() );
	}
};

auto server_handler( weather_data_collection_t & weather_collection )
{
	auto router = std::make_unique< router_t >();
	auto handler = std::make_shared< weather_data_handler_t >( std::ref(weather_collection) );

	auto by = [&]( auto method ) {
		using namespace std::placeholders;
		return std::bind( method, handler, _1, _2 );
	};

	auto method_not_allowed = []( const auto & req, auto ) {
			return req->create_response( restinio::status_method_not_allowed() )
					.connection_close()
					.done();
		};

	// Handlers for '/' path.
	router->http_get( "/", by( &weather_data_handler_t::on_weather_list ) );

	// Disable all other methods for '/'.
	router->add_handler(
			restinio::router::none_of_methods(
					restinio::http_method_get() ),
			"/", method_not_allowed );

	return router;
}

int main()
{
	using namespace std::chrono;

	try
	{
		using traits_t =
			restinio::traits_t<
				restinio::asio_timer_manager_t,
				restinio::single_threaded_ostream_logger_t,
				router_t >;

		weather_data_collection_t weather_collection{
			{ "1", "20240415", "10:15", place_t ("Aarhus N",13.692,19.438), "10.1", 70}
			
		};

		restinio::run(
			restinio::on_this_thread< traits_t >()
				.address( "localhost" )
				.request_handler( server_handler( weather_collection ) )
				.read_next_http_message_timelimit( 10s )
				.write_http_response_timelimit( 1s )
				.handle_request_timeout( 1s ) );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
