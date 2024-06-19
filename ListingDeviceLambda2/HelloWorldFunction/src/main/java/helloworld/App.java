package helloworld;
import java.util.List;
import com.amazonaws.services.iot.AWSIot;
import com.amazonaws.services.iot.AWSIotClientBuilder;
import com.amazonaws.services.iot.model.ListThingsRequest;
import com.amazonaws.services.iot.model.ListThingsResult;
import com.amazonaws.services.iot.model.ThingAttribute;
import com.amazonaws.services.lambda.runtime.Context;
import com.amazonaws.services.lambda.runtime.RequestHandler;

public class App implements RequestHandler<Object, String> {

    @Override
    public String handleRequest(Object input, Context context) {

        // AWSIot 객체를 얻는다.
        AWSIot iot = AWSIotClientBuilder.standard().build();

        // ListThingsRequest 객체 설정.
        ListThingsRequest listThingsRequest = new ListThingsRequest();

        // listThings 메소드 호출하여 결과 얻음.
        ListThingsResult result = iot.listThings(listThingsRequest);

        return getResultStr(result);
    }


    /**
     * ListThingsResult 객체인 result로 부터 ThingName과 ThingArn을 얻어서 Json문자 형식의
     * 응답모델을 만들어 반환한다.
     * {
     * 	"things": [
     *	     {
     *			"thingName": "string",
     *	      	"thingArn": "string"
     *	     },
     *		 ...
     *	   ]
     * }
     */
    private String getResultStr(ListThingsResult result) {
        List<ThingAttribute> things = result.getThings();

        String resultString = "{ \"things\": [";
        for (int i =0; i<things.size(); i++) {
            if (i!=0)
                resultString +=",";
            resultString += String.format("{\"thingName\":\"%s\", \"thingArn\":\"%s\"}",
                    things.get(i).getThingName(),
                    things.get(i).getThingArn());

        }
        resultString += "]}";
        return resultString;
    }

}