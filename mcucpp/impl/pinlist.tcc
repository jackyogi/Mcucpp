
#include <static_if.h>
#include <select_size.h>

namespace IO
{
	using namespace Loki;
	using namespace Loki::TL;

	namespace IoPrivate
	{

////////////////////////////////////////////////////////////////////////////////
// class template CheckSameConfig
// Checks if all ports has the same configuration enum
// PlaceHolderType matchs any type in TList
////////////////////////////////////////////////////////////////////////////////

		template <class TList, class CurrentConfig = typename TList::Head::Configuration>
		struct CheckSameConfigImp;

		template<class TList, class CurrentConfig, class NextConfig>
		struct CheckSameConfigHelper
		{
			static const bool value = false;
		};

		template<class Head, class Tail, class CurrentConfig>
		struct CheckSameConfigHelper<Typelist<Head, Tail>, CurrentConfig, CurrentConfig>
		{
			static const bool value = CheckSameConfigImp<Tail, CurrentConfig>::value;
		};

		template <class CurrentConfig> struct CheckSameConfigImp<NullType, CurrentConfig>
		{
			static const bool value = true;
		};

		template <class Head, class Tail, class CurrentConfig>
		struct CheckSameConfigImp<Typelist<Head, Tail>, CurrentConfig>
		{
			static const bool value = CheckSameConfigHelper<Typelist<Head, Tail>, CurrentConfig, typename Head::Configuration>::value;
		};

		template <class TList>
		struct CheckSameConfig
		{
			static const bool value = CheckSameConfigImp<TList, typename TList::Head::Configuration>::value;
		};

		template <>
		struct CheckSameConfig<NullType>
		{
				static const bool value = true;
		};


////////////////////////////////////////////////////////////////////////////////
		template <class TList, class T, class ConfigT>
		struct ErisePortsHelper;

		template <class TList, class T>
		struct ErasePortsWithConfig
		{
			typedef typename ErisePortsHelper<TList, T, typename TList::Head::Configuration>::Result Result;
		};

		template < class T>
		struct ErasePortsWithConfig<NullType, T>
		{
			typedef NullType Result;
		};

		template <class T, class ConfigT>
		struct ErisePortsHelper<NullType, T, ConfigT>
		{
			typedef NullType Result;
		};

		template <class Head, class Tail, class T>
		struct ErisePortsHelper<Typelist<Head, Tail>, T, T>
		{
			// Go all the way down the list removing the type
			typedef typename ErasePortsWithConfig<Tail, T>::Result Result;
		};

		template <class Head, class Tail, class T, class ConfigT>
		struct ErisePortsHelper<Typelist<Head, Tail>, T, ConfigT>
		{
			// Go all the way down the list removing the type
			typedef Typelist<Head,
					typename ErasePortsWithConfig<Tail, T>::Result>
				Result;
		};

////////////////////////////////////////////////////////////////////////////////

		template<unsigned BitsToShift>
		struct ShiftLeft
		{
			template<class T>
			static T Shift(T value)
			{
				return value << BitsToShift;
			}
		};

		template<unsigned BitsToShift>
		struct ShiftRight
		{
			template<class T>
			static T Shift(T value)
			{
				return value >> BitsToShift;
			}
		};

		enum MapDirection{ValueToPort = 1 , PortToValue = 0};

		template<unsigned First, unsigned Second, MapDirection mapDirection>
		class Shifter
		{
			typedef ShiftRight<First - Second> RightShifter1;
			typedef ShiftRight<Second - First> RightShifter2;

			typedef ShiftLeft<First - Second> LeftShifter1;
			typedef ShiftLeft<Second - First> LeftShifter2;

			static const bool ShiftDirection = First > Second;
			typedef typename StaticIf<ShiftDirection, LeftShifter1, RightShifter2>::Result FirstShifter;
			typedef typename StaticIf<ShiftDirection, RightShifter1, LeftShifter2>::Result SecondShifter;

			typedef typename StaticIf<mapDirection, FirstShifter, SecondShifter>::Result ActualShifter;
		public:
			template<class T>
			static T Shift(T value)
			{
				return ActualShifter::Shift(value);
			}
		};

////////////////////////////////////////////////////////////////////////////////
		template<class PINS> struct GetLastBitPosition;

		template<>
		struct GetLastBitPosition<NullType>
		{
			static const unsigned value = 0;
		};

		template<class Head>
		struct GetLastBitPosition<Typelist<Head, NullType> >
		{
			static const unsigned value = Head::Position;
		};

		template<class Head, class Tail>
		struct GetLastBitPosition<Typelist<Head, Tail> >
		{
			static const unsigned value = GetLastBitPosition<Tail>::value;
		};

////////////////////////////////////////////////////////////////////////////////
// Holds a Pin class and its bit position in value to read/write
////////////////////////////////////////////////////////////////////////////////

		template<class TPIN, uint8_t POSITION>
		struct PinPositionHolder
		{
			typedef TPIN Pin;
			static const unsigned Position = POSITION;
		};

////////////////////////////////////////////////////////////////////////////////
// class template GetPorts
// Converts list of Pin types to Port types
////////////////////////////////////////////////////////////////////////////////

		template <class TList> struct GetPorts;

		template <> struct GetPorts<NullType>
		{
			typedef NullType Result;
		};

		template <class Head, class Tail>
		struct GetPorts< Typelist<Head, Tail> >
		{
		private:
			typedef typename Head::Pin::Port Port;
			typedef typename GetPorts<Tail>::Result L1;
		public:
			typedef Typelist<Port, L1> Result;
		};

////////////////////////////////////////////////////////////////////////////////
		template <class TList> struct GetConfigPins;

		template <> struct GetConfigPins<NullType>
		{
			typedef NullType Result;
		};

		template <class Tail,
				uint8_t ValueBitPosition,
				class PinClass>
		struct GetConfigPins< Typelist<PinPositionHolder<PinClass, ValueBitPosition>, Tail> >
		{
		  typedef typename PinClass::ConfigPort ConfigPort;
		private:
			typedef PinPositionHolder<
			  TPin<ConfigPort, PinClass::Number, ConfigPort>,
						ValueBitPosition> Pin;
			typedef typename GetConfigPins<Tail>::Result L1;
		public:
			typedef Typelist<Pin, L1> Result;
		};

////////////////////////////////////////////////////////////////////////////////
// class template SelectPins
// Assume that TList is type list of PinPositionHolder types.
////////////////////////////////////////////////////////////////////////////////

		template <class TList, template <class> class Predicate>
		struct SelectPins;

		template <template <class> class Predicate>
		struct SelectPins<NullType, Predicate>
		{
			typedef NullType Result;
		};

		template <class Head, class Tail, template <class> class Predicate>
		class SelectPins<Typelist<Head, Tail>, Predicate>
		{
			 typedef typename SelectPins<Tail, Predicate>::Result NotMatch;
			 typedef Typelist<Head,
					typename SelectPins<Tail, Predicate>::Result>
				Match;
			 static const bool IsMatch = Predicate<Head>::value;
		public:
			 typedef typename Select<IsMatch,
				Match,
				NotMatch>::Result Result;
		};

////////////////////////////////////////////////////////////////////////////////
//
// 			Select Predicates
//
////////////////////////////////////////////////////////////////////////////////
// class template TransparentMappedPins
// Selects pins types form pin list with port bit position equals to value bit position
// Assume that TList is type list of PinPositionHolder types.
////////////////////////////////////////////////////////////////////////////////

	template<class Item>
	struct TransparentMappedPins
	{
		static const bool value = Item::Position == Item::Pin::Number;
	};

 	template<class Item>
	struct NotTransparentMappedPins
	{
		static const bool value = Item::Position != Item::Pin::Number;
	};

////////////////////////////////////////////////////////////////////////////////
// class template PinsWithPort
// Selects with specified port
// Assume that TList is type list of PinPositionHolder types.
////////////////////////////////////////////////////////////////////////////////
	template<class Port>
	struct PinsWithPort
	{
		template<class Item>
		struct Result
		{
			static const bool value = IsSameType<Port, typename Item::Pin::Port>::value;
		};
	};


////////////////////////////////////////////////////////////////////////////////
//	Mask for inverted pins
////////////////////////////////////////////////////////////////////////////////

		template <class TList> struct GetInversionMask;

		template <> struct GetInversionMask<NullType>
		{
			enum{value = 0};
		};

		template <class Head, class Tail>
		struct GetInversionMask< Typelist<Head, Tail> >
		{
			enum{value = (Head::Pin::Inverted ? (1 << Head::Pin::Number) : 0) | GetInversionMask<Tail>::value};
		};
////////////////////////////////////////////////////////////////////////////////
// class template GetPortMask
// Computes port bit mask for pin list
// Assume that TList is type list of PinPositionHolder types.
////////////////////////////////////////////////////////////////////////////////

		template <class TList> struct GetPortMask;

		template <> struct GetPortMask<NullType>
		{
			enum{value = 0};
		};

		template <class Head, class Tail>
		struct GetPortMask< Typelist<Head, Tail> >
		{
			enum{value = (1 << Head::Pin::Number) | GetPortMask<Tail>::value};
		};

////////////////////////////////////////////////////////////////////////////////
// class template GetValueMask
// Computes value bit mask for pin list
// Assume that TList is type list of PinPositionHolder types.
////////////////////////////////////////////////////////////////////////////////

		template <class TList> struct GetValueMask;

		template <> struct GetValueMask<NullType>
		{
			enum{value = 0};
		};

		template <class Head, class Tail>
		struct GetValueMask< Typelist<Head, Tail> >
		{
			enum{value = (1 << Head::Position) | GetValueMask<Tail>::value};
		};
////////////////////////////////////////////////////////////////////////////////
// class template GetSerialCount
// Computes number of seqental pins in list
// Assume that TList is type list of PinPositionHolder types.
////////////////////////////////////////////////////////////////////////////////

		template <class TList> struct GetSerialCount;

		template <> struct GetSerialCount<NullType>
		{
			static const int value = 0;
			static const int PinNumber = -1;
			static const int BitPosition = -1;
		};
		template <class Head, class Tail>

		struct GetSerialCount< Typelist<Head, Tail> >
		{
			typedef GetSerialCount<Tail> I;
			static const int PinNumber = Head::Pin::Number;
			static const int BitPosition = Head::Position;
			static const int value =
				((PinNumber == I::PinNumber - 1 &&
				BitPosition == I::BitPosition) ?
				I::value + 1 : 1);
		};
////////////////////////////////////////////////////////////////////////////////
// Returns first Num elements from Typelist
////////////////////////////////////////////////////////////////////////////////
		template <class TList, uint8_t Num> struct TakeFirst;

		template <>
		struct TakeFirst<NullType, 0>
		{
			typedef NullType Result;
		};

		template <class Head, class Tail>
		struct TakeFirst<Typelist<Head, Tail>, 0>
		{
			typedef NullType Result;
		};

		template <class Head, class Tail, uint8_t Num>
		struct TakeFirst<Typelist<Head, Tail>, Num>
		{
			typedef Typelist<Head,
						typename TakeFirst<Tail, Num-1>::Result
						>Result;
		};
////////////////////////////////////////////////////////////////////////////////
// Skip Num first elements from tipelist
////////////////////////////////////////////////////////////////////////////////
		template <class TList, uint8_t Num> struct SkipFirst;

		template <>
		struct SkipFirst<NullType, 0>
		{
			typedef NullType Result;
		};

		template <class Head, class Tail>
		struct SkipFirst<Typelist<Head, Tail>, 0>
		{
			typedef Typelist<Head, Tail> Result;
		};

		template <class Head, class Tail, uint8_t Num>
		struct SkipFirst<Typelist<Head, Tail>, Num>
		{
			typedef typename SkipFirst<Tail, Num-1>::Result Result;
		};

////////////////////////////////////////////////////////////////////////////////
// class template PinWriteIterator
// Iterates througth pin list to compute value to write to their port
// Assume that TList is type list of PinPositionHolder types.
////////////////////////////////////////////////////////////////////////////////

		template <class TList> struct PinWriteIterator;

		template <> struct PinWriteIterator<NullType>
		{
			template<class DataType, class PortDataType>
			static inline PortDataType UppendValue(DataType value, PortDataType result)
			{
				return result;
			}

			template<class DataType, class PortDataType>
			static inline DataType UppendReadValue(PortDataType portValue, DataType result)
			{
				return result;
			}
		};

		template <class Head, class Tail>
		struct PinWriteIterator< Typelist<Head, Tail> >
		{
			//typedef typename Head::Pin::Port::DataT PortDataType;

			template<class DataType, class PortDataType>
			static inline PortDataType UppendValue(DataType value, PortDataType result)
			{
				typedef Typelist<Head, Tail> CurrentList;
				typedef typename SelectPins<CurrentList, TransparentMappedPins>::Result TransparentPins;
				typedef typename SelectPins<CurrentList, NotTransparentMappedPins>::Result NotTransparentPins;
				const int TransparentCount = Length<TransparentPins>::value;

				if(TransparentCount > 1)
				{
					result |= (value &
							GetPortMask<TransparentPins>::value) ^
							GetInversionMask<TransparentPins>::value;

					return PinWriteIterator<NotTransparentPins>::UppendValue(value, result);
				}

				enum{SerialLength = GetSerialCount<CurrentList>::value};

				if(SerialLength >= 2)
				{
					typedef typename TakeFirst<CurrentList, SerialLength>::Result SerialList;
					typedef typename SkipFirst<CurrentList, SerialLength>::Result RestList;

					result |= (Shifter<
							Head::Pin::Number, 	//bit position in port
							Head::Position, 	//bit position in value
							ValueToPort>::Shift(value) &
							GetPortMask<SerialList>::value) ^
							GetInversionMask<SerialList>::value;

					return PinWriteIterator<RestList>::UppendValue(value, result);
				}

				if(Head::Pin::Inverted == false)
				{
					if(value & (1 << Head::Position))
						result |= (1 << Head::Pin::Number);
				}
				else
				{
					if(!(value & (1 << Head::Position)))
						result |= (1 << Head::Pin::Number);
				}

				return PinWriteIterator<Tail>::UppendValue(value, result);
			}

			template<class DataType, class PortDataType>
			static inline DataType UppendReadValue(PortDataType portValue, DataType result)
			{
				typedef Typelist<Head, Tail> CurrentList;
				typedef typename SelectPins<CurrentList, TransparentMappedPins>::Result TransparentPins;
				typedef typename SelectPins<CurrentList, NotTransparentMappedPins>::Result NotTransparentPins;
				const int TransparentCount = Length<TransparentPins>::value;

				if(TransparentCount > 1)
				{
					result |= (portValue &
							GetValueMask<TransparentPins>::value) ^
							GetInversionMask<TransparentPins>::value;

					return PinWriteIterator<NotTransparentPins>::UppendReadValue(portValue, result);
				}
				enum{SerialLength = GetSerialCount<CurrentList>::value};

				if(SerialLength >= 2)
				{
					typedef typename TakeFirst<CurrentList, SerialLength>::Result SerialList;
					typedef typename SkipFirst<CurrentList, SerialLength>::Result RestList;

					typedef Shifter<
							Head::Pin::Number, 	//bit position in port
							Head::Position, 	//bit position in value
							PortToValue> AtctualShifter;

					result |= (AtctualShifter::Shift(portValue) &
					GetValueMask<SerialList>::value) ^
					GetInversionMask<SerialList>::value;
					return PinWriteIterator<RestList>::UppendReadValue(portValue, result);
				}

				if(Head::Pin::Inverted)
				{
					if(!(portValue & (1 << Head::Pin::Number)))
						result |= (1 << Head::Position);

				}else
				{
					if(portValue & (1 << Head::Pin::Number))
						result |= (1 << Head::Position);
				}


				return PinWriteIterator<Tail>::UppendReadValue(portValue, result);
			}
		};
////////////////////////////////////////////////////////////////////////////////
// PinConstWriteIterator
////////////////////////////////////////////////////////////////////////////////
		template <class TList, class DataType, class PortDataType, DataType value> struct PinConstWriteIterator;

		template <class DataType, class PortDataType, DataType value>
		struct PinConstWriteIterator<NullType, DataType, PortDataType, value>
		{
			static const PortDataType PortValue = 0;
		};

		template <class Head, class Tail, class DataType, class PortDataType, DataType value>
		struct PinConstWriteIterator< Typelist<Head, Tail>, DataType, PortDataType, value>
		{
			static const PortDataType PortValue = (value & (1 << Head::Position) ?
					(1 << Head::Pin::Number) : 0) |
					PinConstWriteIterator<Tail, DataType, PortDataType, value>::PortValue;
		};
////////////////////////////////////////////////////////////////////////////////
// class template PortWriteIterator
// Iterates througth port list and write value to them
// Assume that PinList is type list of PinPositionHolder types.
// And PortList is type list of port types.
////////////////////////////////////////////////////////////////////////////////

		template <class PortList, class PinList> struct PortWriteIterator;

		template <class PinList> struct PortWriteIterator<NullType, PinList>
		{
			template<class DataType>
			static void Write(DataType value)
			{   }

			template<class DataType>
			static void Set(DataType value)
			{   }

			template<class DataType>
			static void Clear(DataType value)
			{   }

			template<class DataType>
			static DataType PinRead()
			{
				return 0;
			}

			template<class Configuration, class DataType>
			static void SetConfiguration(Configuration config, DataType mask)
			{	}

			template<class DataType>
			static DataType OutRead()
			{
				return 0;
			}

			// constant writing interface

			template<class DataType, DataType value>
			static void Write()
			{	}

			template<class DataType, DataType value>
			static void Set()
			{	}

			template<class DataType, DataType value>
			static void Clear()
			{	}

			template<class Configuration, class DataType, Configuration config, DataType mask>
			static void SetConfiguration()
			{	}

			template<class DataType, GpioBase::GenericConfiguration config, DataType mask>
			static void SetConfiguration()
			{	}
        };

        template <class Head, class Tail, class PinList>
        struct PortWriteIterator< Typelist<Head, Tail>, PinList>
        {
			//pins on current port
			typedef typename SelectPins<PinList, PinsWithPort<Head>::template Result>::Result Pins;

			enum{Mask = GetPortMask<Pins>::value};

			typedef Head Port; //Head points to current port i the list.

			template<class DataType>
			static void Write(DataType value)
			{
				DataType result = PinWriteIterator<Pins>::UppendValue(value, DataType(0));

				if((int)Length<Pins>::value == (int)Port::Width)// whole port
					Port::Write(result);
				else
				{
					Port::ClearAndSet(Mask, result);
				}

				PortWriteIterator<Tail, PinList>::Write(value);
			}

			template<class DataType>
			static void Set(DataType value)
			{
				DataType result = PinWriteIterator<Pins>::UppendValue(value, DataType(0));
				Port::Set(result);

				PortWriteIterator<Tail, PinList>::Set(value);
			}

			template<class DataType>
			static void Clear(DataType value)
			{
				DataType result = PinWriteIterator<Pins>::UppendValue(value, DataType(0));
				Port::Clear(result);

				PortWriteIterator<Tail, PinList>::Clear(value);
			}

			template<class Configuration, class DataType>
			static void SetConfiguration(Configuration config, DataType mask)
			{
				DataType portMask = PinWriteIterator<Pins>::UppendValue(mask, DataType(0));
				Port::SetConfiguration(portMask, config);
				PortWriteIterator<Tail, PinList>::SetConfiguration(config, mask);
			}

			template<class DataType>
			static void SetConfiguration(GpioBase::GenericConfiguration config, DataType mask)
			{
				DataType portMask = PinWriteIterator<Pins>::UppendValue(mask, DataType(0));
				Port::SetConfiguration(portMask, Port::MapConfiguration(config) );
				PortWriteIterator<Tail, PinList>::SetConfiguration(config, mask);
			}

			template<class DataType>
			static DataType PinRead()
			{
				typename Port::DataT portValue = Port::PinRead();
				return PinWriteIterator<Pins>::UppendReadValue(
							portValue,
							PortWriteIterator<Tail, PinList>::template PinRead<DataType>());
			}

			template<class DataType>
			static DataType OutRead()
			{
				typename Port::DataT portValue = Port::Read();
				return PinWriteIterator<Pins>::UppendReadValue(
							portValue,
							PortWriteIterator<Tail, PinList>::template OutRead<DataType>());
			}

			// constant writing interface

			template<class DataType, DataType value>
			static void Write()
			{
				const typename Port::DataT portValue =
					PinConstWriteIterator<Pins, DataType, typename Port::DataT, value>::PortValue ^
					GetInversionMask<Pins>::value;

				Port::template ClearAndSet<Mask, portValue>();
				PortWriteIterator<Tail, PinList>::template Write<DataType, value>();
			}

			template<class DataType, DataType value>
			static void Set()
			{
				const typename Port::DataT portValue =
					PinConstWriteIterator<Pins, DataType, typename Port::DataT, value>::PortValue ^
					GetInversionMask<Pins>::value;

				Port::template Set<portValue>();
				PortWriteIterator<Tail, PinList>::template Set<DataType, value>();
			}

			template<class DataType, DataType value>
			static void Clear()
			{
				const typename Port::DataT portValue =
					PinConstWriteIterator<Pins, DataType, typename Port::DataT, value>::PortValue^
					GetInversionMask<Pins>::value;

				Port::template Clear<portValue>();
				PortWriteIterator<Tail, PinList>::template Clear<DataType, value>();
			}
        };
////////////////////////////////////////////////////////////////////////////////
// PortConfigurationIterator
////////////////////////////////////////////////////////////////////////////////

		template <class PortList, class PinList, class Configuration, Configuration config>
		struct PortConfigurationIterator;

		template <class PinList, class Configuration, Configuration config>
		struct PortConfigurationIterator<NullType, PinList, Configuration, config>
		{
			template<class DataType, DataType mask>
			static void SetConfiguration()
			{	}
		};

		template <class Head, class Tail, class PinList, class Configuration, Configuration config>
		struct PortConfigurationIterator< Typelist<Head, Tail>, PinList, Configuration, config>
		{
			//pins on current port
			typedef typename SelectPins<PinList, PinsWithPort<Head>::template Result>::Result Pins;
			typedef Head Port; //Head points to current port i the list.

			template<class DataType, DataType mask>
			static void SetConfiguration()
			{
				const typename Port::DataT portValue = PinConstWriteIterator<Pins, DataType, typename Port::DataT, mask>::PortValue;
				Port::template SetConfiguration<portValue, config>();
				PortConfigurationIterator<Tail, PinList, Configuration, config>::template SetConfiguration<DataType, mask>();
			}
		};

		template <class Head, class Tail, class PinList, GpioBase::GenericConfiguration config>
		struct PortConfigurationIterator< Typelist<Head, Tail>, PinList, GpioBase::GenericConfiguration, config>
		{
			//pins on current port
			typedef typename SelectPins<PinList, PinsWithPort<Head>::template Result>::Result Pins;
			typedef Head Port; //Head points to current port i the list.

			template<class DataType, DataType mask>
			static void SetConfiguration()
			{
				const typename Port::DataT portValue =
					PinConstWriteIterator<Pins, DataType, typename Port::DataT, mask>::PortValue;

				const typename Port::Configuration portConfig =
					Port::template MapConfigurationConst<config>::value;

				Port::template SetConfiguration<portValue, portConfig>();

				PortConfigurationIterator<Tail, PinList, GpioBase::GenericConfiguration, config>::
					template SetConfiguration<DataType, mask>();
			}
		};
	}
}