package helloworld;

import com.amazonaws.services.iotdata.AWSIotData;
import com.amazonaws.services.iotdata.AWSIotDataClientBuilder;
import com.amazonaws.services.iotdata.model.GetThingShadowRequest;
import com.amazonaws.services.lambda.runtime.Context;
import com.amazonaws.services.lambda.runtime.RequestHandler;

/**
 * Handler for requests to Lambda function.
 */
public class App implements RequestHandler<Event, String> {

    public String handleRequest(final Event event, final Context context) {

        AWSIotData iotData = AWSIotDataClientBuilder.standard().build();

        GetThingShadowRequest getThingShadowRequest =
                new GetThingShadowRequest()
                        .withThingName(event.device);

        String output = new String(
                iotData.getThingShadow(getThingShadowRequest).getPayload().array());

        return output;
    }
}

class Event {
    public String device;
}
